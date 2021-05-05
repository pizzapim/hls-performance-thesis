#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *ReadFile(char *filename);
int randomrange(int min, int max);
int LoadTestData(char *filename, char **tests, unsigned *pattern_count,
                 unsigned *pattern_sz, unsigned *max_match_count);

#ifdef __cplusplus
}
#endif
