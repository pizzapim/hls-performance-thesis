#include "fmindex.h"
#include "util.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: $ %s <INPUTFILE> <OUTPUTFILE>\n", argv[0]);
    return 1;
  }

  char *s = ReadFile(argv[1]);
  if (!s)
    return 1;

  fm_index *index = FMIndexConstruct(s);
  if (!index) {
    printf("Failed to construct index.\n");
    return 1;
  }

  if (!FMIndexDumpToFile(index, argv[2])) {
    printf("Failed to write FM-index to file.\n");
    return 1;
  }

  FMIndexFree(index);
  free(s);
  return 0;
}
