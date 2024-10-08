#ifndef __GENERATORS_H
#define __GENERATORS_H
#include <stdint.h>

#define INV64 5.421010862427522E-20
#define INV32 2.3283064365386963E-10

typedef unsigned long (*GetBits32Func)(void *param, void *state);

typedef struct {
    const char *name;
    GetBits32Func func;
    void *state;
} GenInfo;

GenInfo alfib_create(void);
GenInfo mwc64_create(void);
GenInfo pcg_create(void);
GenInfo lcg64_create(void);
GenInfo lcg_69069_create(void);
GenInfo mt19937_create(void);
GenInfo xorwow_create(void);
void gen_free(GenInfo *gi);

GenInfo *genlist_create(void);
void genlist_free(GenInfo *genlist);
GenInfo *genlist_find(GenInfo *genlist, const char *name);

#endif
