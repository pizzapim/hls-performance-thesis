#define _GNU_SOURCE

#include "fmindex.h"
#include "rapl.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global variables which can be accessed in function reference.
unsigned pattern_count, pattern_sz, max_match_count;
char *patterns;
fm_index *fm;
float total_time;
unsigned long total_matches = 0;
unsigned long *match_indices;

static void benchmark(void) {
  float time1 = 0., time2 = 0.;
  float start_time, end_time;
  for (unsigned i = 0; i < pattern_count; ++i) {
    ranges_t start, end;
    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindMatchRange(fm, &patterns[i * pattern_sz], pattern_sz, &start,
                          &end);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    time1 += end_time - start_time;

    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindRangeIndices(fm, start, end, &match_indices);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    time2 += end_time - start_time;

    total_matches += end - start;
  }

  total_time = time1 + time2;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: $ %s <FMFILE> <TESTFILE>\n", argv[0]);
    return 1;
  }

  fm = FMIndexReadFromFile(argv[1], 0);
  if (!fm) {
    fprintf(stderr, "Failed to read FM-index from file.\n");
    return 1;
  }

  if (!(LoadTestData(argv[2], &patterns, &pattern_count, &pattern_sz,
                     &max_match_count, 0))) {
    fprintf(stderr, "Could not read test data file.\n");
    return 1;
  }

  match_indices = calloc(max_match_count, sizeof(unsigned long));
  if (!match_indices) {
    fprintf(stderr, "Failed to allocate memory for match indices.\n");
    return 1;
  }

  double total_joules;
  if (rapl_sysfs(benchmark, &total_joules) != 0) {
    fprintf(stderr, "Failed to get energy consumption\n");
    return 1;
  }

  printf("%a %a %lu\n", total_time, total_joules, total_matches);

  free(match_indices);
  free(patterns);
  FMIndexFree(fm);
  return 0;
}
