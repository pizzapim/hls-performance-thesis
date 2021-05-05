#define _GNU_SOURCE

#include "fmindex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: $ %s <FMFILE> <TESTFILE>\n", argv[0]);
    return 1;
  }

  fm_index *index = FMIndexReadFromFile(argv[1], 0);
  if (!index) {
    printf("Failed to read FM-index from file.\n");
    return 1;
  }

  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(argv[2], "r");
  if (!fp) {
    printf("Could not open test file.\n");
    FMIndexFree(index);
    return 1;
  }

  size_t line_len;
  float start_time, end_time, count_time = 0., index_time = 0.;
  ranges_t start, end;
  unsigned long match_count;
  unsigned long *match_indices;
  unsigned pattern_count = 0;
  while ((read = getline(&line, &len, fp)) != -1) {
    ++pattern_count;
    line_len = strlen(line);
    line[line_len - 1] = '\0'; // Remove newline

    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindMatchRange(index, line, &start, &end);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    count_time += end_time - start_time;

    match_count = end - start;
    match_indices = calloc(match_count, sizeof(unsigned long));
    if (!match_indices) {
      printf("Failed to allocate memory for match indices.\n");
      return 1;
    }

    start_time = (float)clock() / CLOCKS_PER_SEC;
    FMIndexFindRangeIndices(index, start, end, &match_indices);
    end_time = (float)clock() / CLOCKS_PER_SEC;
    index_time += end_time - start_time;

    free(match_indices);
  }

  printf("Total count time: %.10f\n", count_time);
  printf("Average count time per pattern: %.10f\n", count_time / pattern_count);
  printf("Total index time: %.10f\n", index_time);
  printf("Average index time per pattern: %.10f\n", index_time / pattern_count);

  free(line);
  fclose(fp);
  FMIndexFree(index);
  return 0;
}
