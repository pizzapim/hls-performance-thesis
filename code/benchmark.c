#define _GNU_SOURCE

#include "fmindex.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: $ %s <FMFILE> <TESTFILE>\n", argv[0]);
    return 1;
  }

  fm_index *index = FMIndexReadFromFile(argv[1], 0);
  if (!index) {
    fprintf(stderr, "Failed to read FM-index from file.\n");
    return 1;
  }

  unsigned pattern_count, pattern_sz, max_match_count;
  char *patterns;

  if (!(LoadTestData(argv[2], &patterns, &pattern_count, &pattern_sz,
                     &max_match_count, 0))) {
    fprintf(stderr, "Could not read test data file.\n");
    return 1;
  }

  unsigned long *match_indices = calloc(max_match_count, sizeof(unsigned long));
  if (!match_indices) {
    fprintf(stderr, "Failed to allocate memory for match indices.\n");
    return 1;
  }

  unsigned long total_matches = 0;
  float range_time = 0., index_time = 0.;
  float start_time, end_time;
  for (unsigned i = 0; i < pattern_count; ++i) {
    ranges_t start, end;
    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindMatchRange(index, &patterns[i * pattern_sz], pattern_sz, &start,
                          &end);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    range_time += end_time - start_time;

    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindRangeIndices(index, start, end, &match_indices);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    index_time += end_time - start_time;

    total_matches += end - start;
  }

  printf("%a %a %lu\n", range_time, index_time, total_matches);

  free(match_indices);
  free(patterns);
  FMIndexFree(index);
  return 0;
}
