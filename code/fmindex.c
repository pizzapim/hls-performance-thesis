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

/* Reduce the size of the given suffix array by only saving
 *  every SA_JUMPS entry.
 * A new array is allocated and the old suffix array is freed.
 * Return NULL on memory allocation error.
 */
sa_t *ReduceSuffixArray(sa_t *sa, size_t sz) {
  size_t reduced_sz = (sz - 1) / SA_JUMPS + 1;

  sa_t *reduced_sa = calloc(reduced_sz, sizeof(sa_t));
  if (!reduced_sa)
    goto cleanup;

  for (size_t i = 0; i * SA_JUMPS < sz; ++i)
    reduced_sa[i] = sa[i * SA_JUMPS];

cleanup:
  free(sa);
  return reduced_sa;
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
  size_t reduced_sz = ((sz - 1) / RANK_JUMPS + 1) * alphabet_sz;

  ranks_t *rank_matrix = calloc(reduced_sz, sizeof(ranks_t));
  if (!rank_matrix) {
    printf("Failed to allocate %lu bytes.\n", sz * alphabet_sz);
    return NULL;
  }

  ranks_t acc[alphabet_sz];

  for (size_t i = 0; i < alphabet_sz; ++i)
    acc[i] = 0;

  for (size_t i = 0; i < sz; ++i) {
    char c = bwt[i];
    for (unsigned j = 0; j < alphabet_sz; ++j)
      if (c == alphabet[j]) {
        ++acc[j];
        break;
      }

    if (i % RANK_JUMPS == 0) {
      for (unsigned j = 0; j < alphabet_sz; ++j) {
        rank_matrix[(i / RANK_JUMPS) * alphabet_sz + j] = acc[j];
      }
    }
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

/* If the rank matrix is reduced to save memory, find the nearest entry that is
 *  available and calculate the rank by moving at most RANK_JUMPS times.
 * When the matrix is not reduced, simply do a array lookup.
 */
#if RANK_JUMPS == 1
#define LookupRank(bwt, sz, rank_matrix, alphabet, alphabet_sz, row, col)      \
  (rank_matrix)[(alphabet_sz) * (row) + (col)]
#else
static ranks_t LookupRank(char *bwt, size_t sz, ranks_t *rank_matrix,
                          char *alphabet, size_t alphabet_sz, unsigned long row,
                          unsigned long col) {
  char target = alphabet[col];
  unsigned long count = 0;
  unsigned long nearest = (row / RANK_JUMPS) * RANK_JUMPS;
  unsigned long cur_row;
  int increment;
  if (row % RANK_JUMPS > RANK_JUMPS / 2 && nearest + RANK_JUMPS < sz) {
    // There is an available row below us, move down.
    increment = 1;
    nearest += RANK_JUMPS;
    cur_row = row + 1;
    while (cur_row % RANK_JUMPS != 0) {
      if (bwt[cur_row] == target)
        ++count;

      ++cur_row;
    }

    if (bwt[cur_row] == target)
      ++count;
  } else {
    // There is an available row above us, move up.
    increment = -1;
    cur_row = row;
    while (cur_row % RANK_JUMPS != 0) {
      if (bwt[cur_row] == target)
        ++count;

      --cur_row;
    }
  }

  // Subtract or add the number of characters found while moving.
  return rank_matrix[(nearest / RANK_JUMPS) * alphabet_sz + col] +
         count * -increment;
}
#endif // RANK_JUMPS == 1

/* Find the range of matches for the given pattern in the F column of the
 *  given FM-index.
 */
void FMIndexFindMatchRange(fm_index *fm, char *pattern, ranges_t *start,
                           ranges_t *end) {
  int p_idx = strlen(pattern) - 1;
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
        range_start + LookupRank(fm->bwt, fm->bwt_sz, fm->ranks, fm->alphabet,
                                 fm->alphabet_sz, *start - 1, alphabet_idx);
    *end =
        range_start + LookupRank(fm->bwt, fm->bwt_sz, fm->ranks, fm->alphabet,
                                 fm->alphabet_sz, *end - 1, alphabet_idx);
    p_idx -= 1;
  }
}

/* Find the indices of the given range of characters in the F column of the
 * given FM-index.
 * Make use of the reduced suffix array, which has only has 1/SA_JUMPS rows.
 * When requesting an index which is not saved in the suffix array,
 *  we jump one character to the left using the LF-mapping until we do.
 * Afterwards, add the number of jumps to the resulting index.
 */
void FMIndexFindRangeIndices(fm_index *fm, ranges_t start, ranges_t end,
                             unsigned long **match_indices) {
  for (unsigned long i = 0; i < end - start; ++i) {
#if SA_JUMPS != 1
    unsigned long idx = start + i;
    unsigned jumps = 0;

    // Jump to the left until we find a row in the suffix array.
    while (idx % SA_JUMPS != 0) {
      int alphabet_idx = string_index(fm->alphabet, fm->bwt[idx]);
      ranges_t range_start = fm->ranges[2 * alphabet_idx];
      idx =
          range_start + LookupRank(fm->bwt, fm->bwt_sz, fm->ranks, fm->alphabet,
                                   fm->alphabet_sz, idx - 1, alphabet_idx);
      jumps++;
    }
    (*match_indices)[i] = (fm->sa[idx / SA_JUMPS] + jumps) % fm->bwt_sz;
#else
    (*match_indices)[i] = fm->sa[start + i];
#endif // SA_JUMPS != 1
  }
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
  if (!(index->sa = ReduceSuffixArray(index->sa, sz)))
    goto error;
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
  fwrite(index->ranks, sizeof(ranks_t),
         ((index->bwt_sz - 1) / RANK_JUMPS + 1) * index->alphabet_sz, f);
  fwrite(index->sa, sizeof(sa_t), (index->bwt_sz - 1) / SA_JUMPS + 1, f);

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
                          FMINDEXRANGESSZ(index) * sizeof(ranges_t), aligned))
    goto error;
  fread(index->ranges, sizeof(ranges_t), FMINDEXRANGESSZ(index), f);

  if (!MaybeMallocAligned((void **)&index->ranks,
                          FMINDEXRANKSSZ(index) * sizeof(ranks_t), aligned))
    goto error;
  fread(index->ranks, sizeof(ranks_t), FMINDEXRANKSSZ(index), f);

  if (!MaybeMallocAligned((void **)&index->sa,
                          FMINDEXSASZ(index) * sizeof(sa_t), aligned))
    goto error;
  fread(index->sa, sizeof(sa_t), FMINDEXSASZ(index), f);

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
