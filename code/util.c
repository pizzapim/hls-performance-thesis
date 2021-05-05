#include "util.h"

#include <stdio.h>
#include <stdlib.h>

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
