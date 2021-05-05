#define _GNU_SOURCE

#include "fmindex.h"
#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline static int string_index(char *s, char c) { return strchr(s, c) - s; }

static int CompareChar(const void *a, const void *b) {
  char i = *(char *)a;
  char j = *(char *)b;

  // Dollar sign is always sorted first.
  if (i == '$')
    return -1;
  if (j == '$')
    return 1;

  return i > j;
}

/* Converts the given string to a newly allocated string
 *  of all distinct characters, sorted in lexicographical order.
 * The dollar sign is always sorted first.
 * Return NULL on memory allocation error.
 */
char *TextToAlphabet(char *text, size_t sz) {
  char *alphabet = calloc(2, sizeof(char));
  if (!alphabet)
    return NULL;

  alphabet[0] = '$';
  alphabet[1] = '\0';
  int len = 2;
  for (size_t i = 0; i < sz; ++i) {
    if (!strchr(alphabet, text[i])) {
      len += 1;
      alphabet = realloc(alphabet, len);
      alphabet[len - 2] = text[i];
      alphabet[len - 1] = '\0';
    }
  }

  qsort(alphabet, len - 1, sizeof(char), &CompareChar);

  return alphabet;
}

static int CompareSuffixArray(const void *a, const void *b, void *arg) {
  sa_t i = *(sa_t *)a;
  sa_t j = *(sa_t *)b;
  char *s = arg;

  while (s[i] != '\0' && s[j] != '\0' && s[i] == s[j]) {
    ++i;
    ++j;
  }

  if (s[i] == '\0')
    return -1;
  if (s[j] == '\0')
    return 1;

  return s[i] > s[j];
}

/* Construct a newly allocated suffix array for the given string.
 * The suffix array holds the index for each suffix in the given string.
 * Then the indices are sorted by the lexicographical ordering of the suffices.
 * Return NULL on memory allocation error.
 */
sa_t *ConstructSuffixArray(char *s, size_t sz) {
  sa_t *suffix_array = calloc(sz + 1, sizeof(sa_t));
  if (!suffix_array)
    return NULL;

  for (size_t i = 0; i < sz + 1; ++i)
    suffix_array[i] = i;

  qsort_r(suffix_array, sz + 1, sizeof(sa_t), &CompareSuffixArray, s);

  return suffix_array;
}

/* Construct the Burrows-Wheeler transform of the given string,
 *  using the corresponding suffix array.
 * Return NULL on memory allocation error.
 */
char *ConstructBWT(char *s, size_t sz, sa_t *suffix_array) {
  char *bwt = calloc(sz + 2, sizeof(char));
  if (!bwt)
    return NULL;

  for (size_t i = 0; i < sz + 1; ++i) {
    sa_t n = suffix_array[i];
    // Index 0 is always the dollar sign.
    bwt[i] = (n) ? s[suffix_array[i] - 1] : '$';
  }
  bwt[sz + 1] = '\0';

  return bwt;
}

/* Construct the rank matrix for the given Burrows-Wheeler transformed string.
 * The rank matrix is an (alphabet X bwt) sized array that holds the accumulated
 *  counts of encountered characters in the BWT.
 * A newly allocated rank matrix is returned, or NULL on memory error.
 */
ranks_t *ConstructRankMatrix(char *bwt, size_t sz, char *alphabet) {
  size_t alphabet_sz = strlen(alphabet);

  ranks_t *rank_matrix = calloc(sz * alphabet_sz, sizeof(ranks_t));
  if (!rank_matrix) {
    printf("Failed to allocate %lu bytes.\n", sz * alphabet_sz);
    return NULL;
  }

  ranks_t acc[alphabet_sz];

  // Initialize counts with zero.
  for (size_t i = 0; i < alphabet_sz; ++i)
    acc[i] = 0;

  for (size_t i = 0; i < sz; ++i) {
    char c = bwt[i];

    // Update accumulator.
    for (unsigned j = 0; j < alphabet_sz; ++j)
      if (c == alphabet[j]) {
        ++acc[j];
        break;
      }

    for (unsigned j = 0; j < alphabet_sz; ++j)
      rank_matrix[i * alphabet_sz + j] = acc[j];
  }

  return rank_matrix;
}

/* Calculate the ranges in the "F column" where each character
 *  in the alphabet appears.
 * Return a (2 X alphabet) matrix where each row means the starting
 *  and ending range for a character.
 * Return NULL on memory error.
 */
ranges_t *ConstructCharacterRanges(char *bwt, size_t sz, char *alphabet) {
  size_t alphabet_sz = strlen(alphabet);
  unsigned long counts[alphabet_sz];

  // Set counts to zero.
  for (size_t i = 0; i < alphabet_sz; ++i)
    counts[i] = 0;

  // Count total amounts of characters.
  for (size_t i = 0; i < sz; ++i)
    counts[string_index(alphabet, bwt[i])]++;

  // Accumulate counts.
  size_t acc = 0;
  for (size_t i = 0; i < alphabet_sz; ++i) {
    counts[i] += acc;
    acc = counts[i];
  }

  // Calculate ranges.
  ranges_t *ranges = calloc(2 * alphabet_sz, sizeof(ranges_t));
  if (!ranges)
    return NULL;

  unsigned long cur_idx = 0;
  for (size_t i = 0; i < alphabet_sz; ++i) {
    ranges[2 * i] = cur_idx;
    ranges[2 * i + 1] = counts[i];
    cur_idx = counts[i];
  }

  return ranges;
}

/* Find the range of matches for the given pattern in the F column of the
 *  given FM-index.
 */
void FMIndexFindMatchRange(fm_index *fm, char *pattern, size_t pattern_sz,
                           ranges_t *start, ranges_t *end) {
  int p_idx = pattern_sz - 1;
  char c = pattern[p_idx];
  // Initial range is all instances of the last character in pattern.
  *start = fm->ranges[2 * string_index(fm->alphabet, c)];
  *end = fm->ranges[2 * string_index(fm->alphabet, c) + 1];

  p_idx -= 1;
  while (p_idx >= 0 && *end > 1) {
    c = pattern[p_idx];
    ranges_t range_start = fm->ranges[2 * string_index(fm->alphabet, c)];
    int alphabet_idx = string_index(fm->alphabet, c);
    *start =
        range_start + fm->ranks[fm->alphabet_sz * (*start - 1) + alphabet_idx];
    *end = range_start + fm->ranks[fm->alphabet_sz * (*end - 1) + alphabet_idx];
    p_idx -= 1;
  }
}

/* Find the matching indices in the original text for the given
 *  range in the "F column" of the Burrows-Wheeler matrix.
 */
void FMIndexFindRangeIndices(fm_index *fm, ranges_t start, ranges_t end,
                             unsigned long **match_indices) {
  for (unsigned long i = 0; i < end - start; ++i)
    (*match_indices)[i] = fm->sa[start + i];
}

fm_index *FMIndexConstruct(char *s) {
  fm_index *index = malloc(sizeof(fm_index));
  if (!index)
    return NULL;
  memset(index, 0, sizeof(fm_index));

  size_t sz = strlen(s);
  if (!(index->alphabet = TextToAlphabet(s, sz)))
    goto error;
  index->alphabet_sz = strlen(index->alphabet);
  if (!(index->sa = ConstructSuffixArray(s, sz)))
    goto error;
  if (!(index->bwt = ConstructBWT(s, sz, index->sa)))
    goto error;
  ++sz; // Because of the added dollar sign.
  index->bwt_sz = sz;
  if (!(index->ranks = ConstructRankMatrix(index->bwt, sz, index->alphabet)))
    goto error;
  if (!(index->ranges =
            ConstructCharacterRanges(index->bwt, sz, index->alphabet)))
    goto error;

  return index;

error:
  if (index->alphabet)
    free(index->alphabet);
  if (index->sa)
    free(index->sa);
  if (index->bwt)
    free(index->bwt);
  if (index->ranks)
    free(index->ranks);
  if (index->ranges)
    free(index->ranges);

  free(index);
  return NULL;
}

void FMIndexFree(fm_index *index) {
  free(index->alphabet);
  free(index->sa);
  free(index->bwt);
  free(index->ranks);
  free(index->ranges);
  free(index);
}

int FMIndexDumpToFile(fm_index *index, char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f)
    return 0;

  fwrite(&index->bwt_sz, sizeof(index->bwt_sz), 1, f);
  fwrite(index->bwt, sizeof(char), index->bwt_sz, f);
  fwrite(&index->alphabet_sz, sizeof(index->alphabet_sz), 1, f);
  fwrite(index->alphabet, sizeof(char), index->alphabet_sz, f);
  fwrite(index->ranges, sizeof(ranges_t), 2 * index->alphabet_sz, f);
  fwrite(index->ranks, sizeof(ranks_t), index->bwt_sz * index->alphabet_sz, f);
  fwrite(index->sa, sizeof(sa_t), index->bwt_sz, f);

  fclose(f);
  return 1;
}

static int MaybeMallocAligned(void **mem, size_t sz, int aligned) {
  if (aligned)
    return posix_memalign(mem, 4096, sz) == 0;
  else
    return (*mem = malloc(sz)) != NULL;
}

fm_index *FMIndexReadFromFile(char *filename, int aligned) {
  FILE *f = fopen(filename, "r");
  if (!f)
    return NULL;

  fm_index *index;

  if (!(index = calloc(1, sizeof(fm_index))))
    goto error;

  fread(&index->bwt_sz, sizeof(index->bwt_sz), 1, f);

  if (!MaybeMallocAligned((void **)&index->bwt,
                          (index->bwt_sz + 1) * sizeof(char), aligned))
    goto error;
  fread(index->bwt, sizeof(char), index->bwt_sz, f);
  index->bwt[index->bwt_sz] = '\0';

  fread(&index->alphabet_sz, sizeof(index->alphabet_sz), 1, f);

  if (!MaybeMallocAligned((void **)&index->alphabet,
                          (index->alphabet_sz + 1) * sizeof(char), aligned))
    goto error;
  fread(index->alphabet, sizeof(char), index->alphabet_sz, f);
  index->alphabet[index->alphabet_sz] = '\0';

  if (!MaybeMallocAligned((void **)&index->ranges,
                          2 * index->alphabet_sz * sizeof(ranges_t), aligned))
    goto error;
  fread(index->ranges, sizeof(ranges_t), 2 * index->alphabet_sz, f);

  if (!MaybeMallocAligned((void **)&index->ranks,
                          index->bwt_sz * index->alphabet_sz * sizeof(ranks_t),
                          aligned))
    goto error;
  fread(index->ranks, sizeof(ranks_t), index->bwt_sz * index->alphabet_sz, f);

  if (!MaybeMallocAligned((void **)&index->sa, index->bwt_sz * sizeof(sa_t),
                          aligned))
    goto error;
  fread(index->sa, sizeof(sa_t), index->bwt_sz, f);

  fclose(f);
  return index;

error:
  if (index) {
    if (index->bwt)
      free(index->bwt);
    if (index->alphabet)
      free(index->alphabet);
    if (index->ranges)
      free(index->ranges);
    if (index->ranks)
      free(index->ranks);
    if (index->sa)
      free(index->sa);
    free(index);
  }

  fclose(f);
  return NULL;
}
