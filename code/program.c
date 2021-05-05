#include "fmindex.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

int main() {
  char *s = "ALALA";
  char *pattern = "AL";

  printf("> Constructing FM-index...\n");
  fm_index *index = FMIndexConstruct(s);
  if (!index) {
    printf("Failed to allocate memory for index...\n");
    return 1;
  }

  printf("> FM-index constructed.\n");

  printf("BWT: \"%s\"\n", index->bwt);

  ranges_t start, end;
  FMIndexFindMatchRange(index, pattern, &start, &end);
  unsigned long match_count = end - start;
  unsigned long *match_indices = calloc(match_count, sizeof(unsigned long));
  FMIndexFindRangeIndices(index, start, end, &match_indices);

  printf("\nPattern \"%s\" occurs: %lu times\nIndices: ", pattern, match_count);
  for (unsigned long i = 0; i < match_count; ++i)
    printf("%lu ", match_indices[i]);
  printf("\n");
  if (match_count)
    printf("%s\n", s);
  for (unsigned long i = 0; i < index->bwt_sz - 1; ++i) {
    int found = 0;
    for (unsigned long j = 0; j < match_count; ++j)
      if (match_indices[j] == i)
        found = 1;

    printf("%c", (found) ? '^' : ' ');
  }
  printf("\n");

  free(match_indices);
  FMIndexFree(index);
  return 0;
}
