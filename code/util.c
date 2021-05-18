#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://stackoverflow.com/questions/2029103/#2029227
char *ReadFile(char *filename) {
  char *source = NULL;
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    printf("Failed to open file.\n");
    return NULL;
  }

  if (fseek(fp, 0L, SEEK_END) == 0) {
    long bufsize = ftell(fp);
    if (bufsize == -1) {
      printf("Failed to determine file size.\n");
      return NULL;
    }
    source = malloc(sizeof(char) * (bufsize + 1));
    if (!source) {
      printf("Failed to allocate space for file contents.\n");
      return NULL;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
      printf("Failed to move to the start of the file.\n");
      return NULL;
    }

    size_t newLen = fread(source, sizeof(char), bufsize, fp);
    if (ferror(fp) != 0) {
      printf("Error reading from file.\n");
      return NULL;
    }

    source[newLen++] = '\0';
  }

  fclose(fp);
  return source;
}

// https://stackoverflow.com/questions/1202687/18386648#18386648
int randomrange(int min, int max) {
  return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int LoadTestData(char *filename, char **tests, unsigned *pattern_count,
                 unsigned *pattern_sz, unsigned *max_match_count, int aligned) {
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  if (!(fp = fopen(filename, "r")))
    return 0;

  // Read max match count.
  if ((read = getline(&line, &len, fp)) == -1)
    return 0;
  *max_match_count = atoi(line);

  // Read test count.
  if ((read = getline(&line, &len, fp)) == -1)
    return 0;
  *pattern_count = atoi(line);

  // Read test length.
  if ((read = getline(&line, &len, fp)) == -1)
    return 0;
  *pattern_sz = atoi(line);

  if (!MaybeMallocAligned((void **)tests,
                          *pattern_count * *pattern_sz * sizeof(char), aligned))
    return 0;

  // Read each test on a seperate line.
  for (unsigned i = 0; i < *pattern_count; ++i) {
    if ((read = getline(&line, &len, fp)) == -1)
      return 0;
    memcpy(&(*tests)[i * *pattern_sz], line, *pattern_sz);
  }

  fclose(fp);
  free(line);

  return 1;
}

int MaybeMallocAligned(void **mem, size_t sz, int aligned) {
  if (aligned)
    return posix_memalign(mem, 4096, sz) == 0;
  else
    return (*mem = malloc(sz)) != NULL;
}
