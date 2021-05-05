extern "C" {

#include <stdlib.h>
#include <string.h>

typedef unsigned ranges_t;
typedef unsigned ranks_t;
typedef unsigned sa_t;

inline static int string_index(char *s, char c) {
  int i = 0;
  while (1) {
    if (s[i] == c)
      return i;
    ++i;
  }
}

void fmindex(char *bwt, char *alphabet, ranks_t *ranks, sa_t *sa,
             ranges_t *ranges, char *patterns, unsigned long *out,
             size_t bwt_sz, size_t alphabet_sz, unsigned pattern_count,
             unsigned pattern_sz, unsigned out_sz) {
  for (unsigned i = 0; i < pattern_count; ++i) {
    char pattern[pattern_count];
    memcpy(pattern, &patterns[i * pattern_sz], pattern_sz * sizeof(char));
    int p_idx = pattern_sz - 1;
    char c = pattern[p_idx];
    ranges_t start = ranges[2 * string_index(alphabet, c)];
    ranges_t end = ranges[2 * string_index(alphabet, c) + 1];

    p_idx -= 1;
    while (p_idx >= 0 && end > 1) {
      c = pattern[p_idx];
      ranges_t range_start = ranges[2 * string_index(alphabet, c)];
      int alphabet_idx = string_index(alphabet, c);
      start = range_start + ranks[alphabet_sz * (start - 1) + alphabet_idx];
      end = range_start + ranks[alphabet_sz * (end - 1) + alphabet_idx];
      p_idx -= 1;
    }

    unsigned long local_out[out_sz];

    for (unsigned j = 0; j < out_sz; ++j) {
      local_out[j] = 0;
    }

    for (unsigned j = 0; j < end - start; ++j) {
      local_out[j] = sa[start + j];
    }
    memcpy(&out[i * out_sz], local_out, out_sz * sizeof(unsigned long));
  }
}
}
