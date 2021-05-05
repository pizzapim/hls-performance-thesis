extern "C" {

#include <stdlib.h>

typedef unsigned ranges_t;
typedef unsigned ranks_t;
typedef unsigned sa_t;

#define LookupRank(rank_matrix, alphabet_sz, row, col)                         \
  (rank_matrix)[(alphabet_sz) * (row) + (col)]

inline static int string_index(char *s, char c) {
  int i = 0;
  while (1) {
    if (s[i] == c)
      return i;
    ++i;
  }
}

void fmindex(char *bwt, char *alphabet, ranks_t *ranks, sa_t *sa,
             ranges_t *ranges, size_t bwt_sz, size_t alphabet_sz, size_t *out) {
#pragma HLS INTERFACE m_axi port = bwt bundle = aximm1
#pragma HLS INTERFACE m_axi port = alphabet bundle = aximm2
#pragma HLS INTERFACE m_axi port = ranks bundle = aximm3
#pragma HLS INTERFACE m_axi port = sa bundle = aximm2
#pragma HLS INTERFACE m_axi port = ranges bundle = aximm2
#pragma HLS INTERFACE m_axi port = out bundle = aximm1
  char *pattern = "bee";
  int p_idx = 2;
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

  out[0] = (end >= start) ? end - start : 0;
}
}
