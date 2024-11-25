#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <time.h>


typedef unsigned char u8;

#if UINT_MAX == 0xFFFFFFFF
typedef unsigned int u32;
typedef int i32;
#else
typedef unsigned long u32;
typedef long i32;
#endif

typedef u32 (*GetBits32Func)(void *state);

typedef struct {
    GetBits32Func get_bits32;
    void *state;
} Generator32State;


u32 lcg69069func(void *state)
{
    u32 *x = (u32 *) state;
    *x = 69069 * (*x) + 12345;
    return *x;
}

static int cmp_u32(const void *aptr, const void *bptr)
{
    u32 aval = *((u32 *) aptr), bval = *((u32 *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}


void gen_tests(Generator32State *obj)
{
    int n = 4096, i, ii, ndups = 0, nsamples = 4096;    
    u32 *x = calloc(n, sizeof(u32));
    for (ii = 0; ii < nsamples; ii++) {
        for (i = 0; i < n; i++) {
            x[i] = obj->get_bits32(obj->state);
        }
        qsort(x, n, sizeof(u32), cmp_u32);
        for (i = 0; i < n - 1; i++) {
            x[i] = x[i + 1] - x[i];
        }
        qsort(x, n - 1, sizeof(u32), cmp_u32);

        for (i = 0; i < n - 2; i++) {
            if (x[i] == x[i + 1])
                ndups++;
        }
    }
    printf("-->%d\n", ndups);
    free(x);
}


int main()
{
    u32 x = time(NULL);
    Generator32State lcg32;
    lcg32.state = &x;
    lcg32.get_bits32 = lcg69069func;

    gen_tests(&lcg32);

    printf("%d\n", (int) sizeof(u32));
    printf("%d\n", (int) sizeof(i32));

}
