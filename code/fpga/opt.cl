#define MAX_PATTERN_SZ 8
#define MAX_ALPHABET_SZ 97
#define MAX_RANGES_SZ 194
#define PATTERN_COUNT 7500

inline static int string_index(__local char *s, char c) {
  int i = 0;
  while (1) {
    if (s[i] == c)
      return i;
    ++i;
  }
}

kernel __attribute__((reqd_work_group_size(1,1,1)))
void fmindex(__global char *bwt, __global char *alphabet, __global unsigned *ranks, __global unsigned *sa,
           __global unsigned *ranges, __global char *patterns, __global unsigned long *out, size_t bwt_sz,
           size_t alphabet_sz, unsigned pattern_count, unsigned pattern_sz,
           unsigned out_sz) {
  __local char pattern[MAX_PATTERN_SZ];
  __local char local_alphabet[MAX_ALPHABET_SZ];
  __local unsigned local_ranges[MAX_RANGES_SZ];

  __attribute__((xcl_pipeline_loop(1)))
  for (unsigned j = 0; j < alphabet_sz; ++j)
    local_alphabet[j] = alphabet[j];

  __attribute__((xcl_pipeline_loop(1)))
  for (unsigned j = 0; j < 2 * alphabet_sz; ++j)
    local_ranges[j] = ranges[j];

  __attribute__((xcl_loop_tripcount(PATTERN_COUNT, PATTERN_COUNT, PATTERN_COUNT)))
  __attribute__((xcl_pipeline_loop(1)))
  for (unsigned i = 0; i < pattern_count; ++i) {
    __attribute__((xcl_pipeline_loop(1)))
    for (unsigned j = 0; j < pattern_sz; ++j)
      pattern[j] = patterns[i * pattern_sz + j];

    int p_idx = pattern_sz - 1;
    char c = pattern[p_idx];
    int alphabet_idx = string_index(local_alphabet, c);
    unsigned start = local_ranges[2 * alphabet_idx];
    unsigned end = local_ranges[2 * alphabet_idx + 1];

    p_idx -= 1;
    while (p_idx >= 0 && end > 1) {
      c = pattern[p_idx];
      alphabet_idx = string_index(local_alphabet, c);
      unsigned range_start = local_ranges[2 * alphabet_idx];
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
