#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define FMINDEXRANKSSZ(idx) ((((idx)->bwt_sz - 1) / RANK_JUMPS + 1) * (idx)->alphabet_sz)
#define FMINDEXRANGESSZ(idx) (2 * (idx)->alphabet_sz)
#define FMINDEXSASZ(idx) (((idx)->bwt_sz - 1) / SA_JUMPS + 1)

typedef unsigned ranges_t;
typedef unsigned ranks_t;
typedef unsigned sa_t;
typedef struct fm_index {
  char *bwt;
  size_t bwt_sz;
  char *alphabet;
  size_t alphabet_sz;
  ranks_t *ranks;
  sa_t *sa;
  ranges_t *ranges;
} fm_index;

fm_index *FMIndexConstruct(char *s);
void FMIndexFree(fm_index *index);

fm_index *FMIndexReadFromFile(char *filename, int aligned);
int FMIndexDumpToFile(fm_index *index, char *filename);

void FMIndexFindMatchRange(fm_index *fm, char *pattern, ranges_t *start,
                           ranges_t *end);
void FMIndexFindRangeIndices(fm_index *fm, ranges_t start, ranges_t end,
                             unsigned long **match_indices);

#ifdef __cplusplus
}
#endif
