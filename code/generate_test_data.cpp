#include <random>
#include <stdio.h>
#include <string.h>

#include "util.h"

int main(int argc, char **argv) {
  if (argc < 5) {
    printf("Usage: $ %s <INPUTFILE> <OUTPUTFILE> <TESTCOUNT> <TESTLENGTH>\n",
           argv[0]);
    return 1;
  }

  int count = atoi(argv[3]);
  int length = atoi(argv[4]);

  char *s = ReadFile(argv[1]);
  if (!s)
    return 1;

  size_t sz = strlen(s);

  std::random_device rand_dev;
  std::mt19937 generator(rand_dev());
  std::uniform_int_distribution<unsigned long> distr(0, sz - length);

  FILE *f = fopen(argv[2], "w");
  if (!f) {
    printf("Failed to open output file.\n");
    return 1;
  }

  for (int i = 0; i < count; ++i) {
    unsigned long index = distr(generator);
    fprintf(f, "%.*s\n", length, &s[index]);
  }

  return 0;
}
