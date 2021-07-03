#define LOCAL_SIZE 300
#define MAX_PATTERN_SZ 8
#define MAX_ALPHABET_SZ 97
#define MAX_RANGES_SZ (2 * MAX_ALPHABET_SZ)
#define PATTERNS_SZ (MAX_PATTERN_SZ * LOCAL_SIZE)

inline static int string_index(__private char *s, char c) {
  int i = 0;
  while (1) {
    if (s[i] == c)
      return i;
    ++i;
  }
}

kernel
__attribute__((reqd_work_group_size(LOCAL_SIZE, 1, 1)))
__attribute__((xcl_zero_global_work_offset))
void fmindex(__global char *bwt,
             __global char *alphabet,
             __global unsigned *ranks,
             __global unsigned *sa,
             __global unsigned *ranges,
             __global char *patterns,
             __global unsigned long *out,
             size_t bwt_sz, size_t alphabet_sz, unsigned pattern_count,
             unsigned pattern_sz, unsigned out_sz) {
  int group_id = get_group_id(0);

  __local char _patterns[PATTERNS_SZ];
  __attribute__((xcl_pipeline_loop(1)))
  for (unsigned i = 0; i < PATTERNS_SZ; ++i)
    _patterns[i] = patterns[group_id * LOCAL_SIZE * pattern_sz + i];

  __attribute__((xcl_pipeline_workitems)) {
    int work_id = get_global_id(0);
    int local_id = get_local_id(0);

    __private char _alphabet[MAX_ALPHABET_SZ];
    __attribute__((xcl_pipeline_loop(1)))
    for (unsigned i = 0; i < alphabet_sz; ++i)
      _alphabet[i] = alphabet[i];

    __private unsigned _ranges[MAX_RANGES_SZ];
    __attribute__((xcl_pipeline_loop(1)))
    for (unsigned i = 0; i < 2 * alphabet_sz; ++i)
      _ranges[i] = ranges[i];

    int p_idx = pattern_sz - 1;
    char c = _patterns[local_id * pattern_sz + p_idx];
    int alphabet_idx = string_index(_alphabet, c);
    unsigned start = _ranges[2 * alphabet_idx];
    unsigned end = _ranges[2 * alphabet_idx + 1];
  
    p_idx -= 1;
    while (p_idx >= 0 && end > 1) {
      c = _patterns[local_id * pattern_sz + p_idx];
      alphabet_idx = string_index(_alphabet, c);
      unsigned range_start = _ranges[2 * alphabet_idx];
      start = range_start + ranks[alphabet_sz * (start - 1) + alphabet_idx];
      end = range_start + ranks[alphabet_sz * (end - 1) + alphabet_idx];
      p_idx -= 1;
    }
  
    unsigned long match_count = end - start;
    out[work_id * out_sz] = match_count;

    __attribute__((xcl_pipeline_loop(1)))
    for (unsigned i = 0; i < match_count; ++i)
      out[work_id * out_sz + i + 1] = sa[start + i];
  }
}
