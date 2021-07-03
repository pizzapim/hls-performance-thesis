#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

char *ReadFile(char *filename);
int randomrange(int min, int max);
int LoadTestData(char *filename, char **tests, unsigned *pattern_count,
                 unsigned *pattern_sz, unsigned *max_match_count, int aligned);
int MaybeMallocAligned(void **mem, size_t sz, int aligned);

#ifdef __cplusplus
}
#endif
