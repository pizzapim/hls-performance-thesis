#include "fmindex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: $ %s <FMINDEXFILE>\n", argv[0]);
    return 1;
  }

  fm_index *index = FMIndexReadFromFile(argv[1], 0);
  if (!index) {
    printf("Could not read FM-index from file.\n");
    return 1;
  }

  int input_len = 256;
  char input[input_len];
  do {
    printf("Type your query: ");
    fgets(input, input_len, stdin);
    if (!strlen(input))
      break;
    input[strlen(input) - 1] = '\0';

    ranges_t start, end;
    FMIndexFindMatchRange(index, input, strlen(input), &start, &end);
    unsigned long match_count = end - start;
    unsigned long *match_indices = calloc(match_count, sizeof(unsigned long));
    FMIndexFindRangeIndices(index, start, end, &match_indices);

    printf("Indices: ");
    for (unsigned long i = 0; i < match_count; ++i) {
      printf("%lu ", match_indices[i]);
    }
    printf("\n");
    printf("Found %lu matches.\n", match_count);
    free(match_indices);
  } while (1);

  FMIndexFree(index);
  return 0;
}
