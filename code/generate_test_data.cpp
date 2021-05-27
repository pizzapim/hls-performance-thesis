#include <random>
#include <stdio.h>
#include <string.h>

#include "fmindex.h"
#include "util.h"

int main(int argc, char **argv) {
  if (argc < 6) {
    printf("Usage: $ %s <TEXTFILE> <FMFILE> <OUTPUTFILE> <TESTCOUNT> <TESTLENGTH>\n",
           argv[0]);
    return 1;
  }

  int count = atoi(argv[4]);
  int length = atoi(argv[5]);

  char *s = ReadFile(argv[1]);
  if (!s)
    return 1;

  fm_index *index = FMIndexReadFromFile(argv[2], 0);
  if (!index) {
    fprintf(stderr, "Error reading FM-index from file\n");
    return 1;
  }

  size_t sz = strlen(s);

  unsigned long seed = time(NULL);
  fprintf(stderr, "Seed: %lu\n", seed);

  std::random_device rand_dev;
  std::mt19937 generator(seed);
  std::uniform_int_distribution<unsigned long> distr(0, sz - length);

  unsigned long indices[count];
  unsigned max_match_count = 1;
  char pattern[length + 1];
  pattern[length] = '\0';

  for (int i = 0; i < count; ++i) {
    // Make sure the pattern does not contain newlines as it would
    //  mess with the formatting of the file.
    // Skews the distributivity a bit, but shouldn't matter really.
    unsigned long idx;
    int contains_newline;
    do {
      idx = distr(generator);
      contains_newline = 0;
      for (int i = 0; i < length; ++i)
        if (s[idx + i] == '\n')
          contains_newline = 1;
    } while (contains_newline);

    indices[i] = idx;
    memcpy(pattern, &s[idx], length);
    ranges_t start, end;
    FMIndexFindMatchRange(index, pattern, length, &start, &end);
    if (end - start > max_match_count)
      max_match_count = end - start;
  }

  FILE *f = fopen(argv[3], "w");
  if (!f) {
    fprintf(stderr, "Failed to open output file.\n");
    return 1;
  }

  fprintf(f, "%i\n", max_match_count);
  fprintf(f, "%i\n", count);
  fprintf(f, "%i\n", length);
  for (int i = 0; i < count; ++i) {
    fprintf(f, "%.*s\n", length, &s[indices[i]]);
  }

  FMIndexFree(index);

  return 0;
}
