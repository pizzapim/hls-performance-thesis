#define MAX_PATTERN_SZ 8
#define MAX_MATCH_COUNT                                                        \
  227386 // Empirically found to be average match count in worst case

inline static int string_index(__global char *s, char c) {
  int i = 0;
  while (1) {
    if (s[i] == c)
      return i;
    ++i;
  }
}

kernel __attribute__((reqd_work_group_size(1,1,1)))
void fmindex(__global char *bwt,
             __global char *alphabet,
             __global unsigned *ranks,
             __global unsigned *sa,
             __global unsigned *ranges,
             __global char *patterns,
             __global unsigned long *out,
             size_t bwt_sz, size_t alphabet_sz, unsigned pattern_count,
             unsigned pattern_sz, unsigned out_sz) {
  for (unsigned i = 0; i < pattern_count; ++i) {
    int p_idx = pattern_sz - 1;
    char c = patterns[i * pattern_sz + p_idx];
    unsigned start = ranges[2 * string_index(alphabet, c)];
    unsigned end = ranges[2 * string_index(alphabet, c) + 1];

    p_idx -= 1;
    while (p_idx >= 0 && end > 1) {
      c = patterns[i * pattern_sz + p_idx];
      unsigned range_start = ranges[2 * string_index(alphabet, c)];
      int alphabet_idx = string_index(alphabet, c);
      start = range_start + ranks[alphabet_sz * (start - 1) + alphabet_idx];
      end = range_start + ranks[alphabet_sz * (end - 1) + alphabet_idx];
      p_idx -= 1;
    }

    unsigned long match_count = end - start;
    out[i * out_sz] = match_count;
    for (unsigned j = 0; j < match_count; ++j)
      out[i * out_sz + j + 1] = sa[start + j];
  }
}