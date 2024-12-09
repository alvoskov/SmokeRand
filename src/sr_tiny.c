/*

set path=%path%;c:\c_prog\dm\bin
path
dmc sr_tiny.c -c -2 -o -ms -I../include -osr_tiny.obj
dmc specfuncs.c -c -2 -o -ms -I../include -ospecfuncs.obj
dmc sr_tiny.obj specfuncs.obj -osr_tiny.exe

*/
#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef unsigned short u16;

typedef u32 (*GetBits32Func)(void *state);

typedef struct {
    GetBits32Func get_bits32;
    void *state;
} Generator32State;


/*----------------------------------------------------------*/

/**
 * @brief MWC1616X state.
 */
typedef struct {
    u32 z;
    u32 w;
} Mwc1616xState;


u32 mwc1616x_func(void *state)
{
    Mwc1616xState *obj = state;
    u16 z_lo = obj->z & 0xFFFF, z_hi = obj->z >> 16;
    u16 w_lo = obj->w & 0xFFFF, w_hi = obj->w >> 16;
    obj->z = 61578ul * z_lo + z_hi;
    obj->w = 63885ul * w_lo + w_hi;
    return ((obj->z << 16) | (obj->z >> 16)) ^ obj->w;
}

void Mwc1616xState_init(Mwc1616xState *obj, u32 seed)
{
    obj->z = (seed & 0xFFFF) | (1ul << 16ul);
    obj->w = (seed >> 16) | (1ul << 16ul);
}


/*----------------------------------------------------------*/

u32 lcg69069func(void *state)
{
    u32 *x = (u32 *) state;
    *x = 69069 * (*x) + 12345;
    return *x;
}


/*---------------------------------------------------------*/

/*
static int cmp_u32(const void *aptr, const void *bptr)
{
    u32 aval = *((u32 *) aptr), bval = *((u32 *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}
*/


void insertsort(u32 *x, int begin, int end)
{
    for (int i = begin + 1; i <= end; i++) {
        u32 xi = x[i];
        int j = i;
        while (j > begin && x[j - 1] > xi) {
            x[j] = x[j - 1];
            j--;
        }
        x[j] = xi;
    }
}

void quicksort(u32 *v, int begin, int end)
{
    int i = begin, j = end, med = (begin + end) / 2;
    u32 pivot = v[med];
    while (i <= j) {
        if (v[i] < pivot) {
            i++;
        } else if (v[j] > pivot) {
            j--;
        } else {
            u32 tmp = v[i];
            v[i++] = v[j];
            v[j--] = tmp;
        }
    }
    if (begin < j) {
        if (j - begin > 12) {
            quicksort(v, begin, j);
        } else {
            insertsort(v, begin, j);
        }
    }
    if (end > i) {
        if (end - i > 12) {
            quicksort(v, i, end);
        } else {
            insertsort(v, i, end);
        }
    }
}


int get_ndups(u32 *x, int n)
{
    int i, ndups = 0;
    /*qsort(x, n, sizeof(u32), cmp_u32);*/
    quicksort(x, 0, n - 1);
/*
    for (i = 1; i < n - 1; i++) {
        if (x[i] < x[i - 1]) {
            printf("!!!!!!!!!!!!! %d\n", i);
        }
    }
*/

    for (i = 0; i < n - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    /*qsort(x, n - 1, sizeof(u32), cmp_u32);*/
    quicksort(x, 0, n - 2);

    for (i = 0; i < n - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}


void gen_tests(Generator32State *obj)
{
    const double lambda = 4.0;
    int n = 4096, i, ii, ndups = 0, ndups_dec, nsamples = 512;
    int pos_dec = 0;
    u32 u_dec = 0;
    u32 *x = calloc(n, sizeof(u32));
    u32 *x_dec = calloc(n, sizeof(u32));
    for (ii = 0; ii < nsamples; ii++) {
        for (i = 0; i < n; i++) {
            u32 u = obj->get_bits32(obj->state);
            x[i] = u;
            /* TODO: MAKE BYTE COUNTING */

            /* Making subsample for birthday spacings with decimation
               Take only every 64th point and use lower 4 bits from each.
               We will analyse 8-tuples made of 4-bit elements. */
            if ((i & 0x3F) == 0 && pos_dec < n) {
                u_dec <<= 4;
                u_dec |= u & 0xF;
                /* Check if 8 elements are already collected */
                if ((i & 0x1C0) == 0x1C0) {
                    x_dec[pos_dec++] = u_dec;
                    u_dec = 0;
                }
            }
        }
        ndups += get_ndups(x, n);
        printf("%d of %d\r", ii, nsamples);
    }

    ndups_dec = get_ndups(x_dec, n);
    printf("\n");
    printf("  bspace32_1d\n");
    printf("    %d\n", ndups);
    printf("    %g\n", poisson_pvalue(ndups, nsamples * lambda));
    printf("  bspace4_8d_dec\n");
    printf("    %d\n", ndups_dec);
    printf("    %g\n", poisson_pvalue(ndups_dec, lambda));
    free(x);
}


int main(int argc, char *argv[])
{
    u32 x = time(NULL), tic, toc;
    Generator32State gen;
    Mwc1616xState mwc1616x;
    if (argc < 2) {
        printf("Usage: sr_tiny gen_name\n");
        printf("  gen_name = lcg69069, mwc1616x\n");
        return 0;
    }
    if (!strcmp("lcg69069", argv[1])) {
        gen.state = &x;
        gen.get_bits32 = lcg69069func;
    } else if (!strcmp("mwc1616x", argv[1])) {
        Mwc1616xState_init(&mwc1616x, x);
        gen.state = &mwc1616x;
        gen.get_bits32 = mwc1616x_func;
    } else {
        fprintf(stderr, "Unknown generator %s\n", argv[1]);
        return 1;
    }

    tic = time(NULL);
    gen_tests(&gen);
    toc = time(NULL);
    printf("::%d::\n", toc - tic);

    printf("%d\n", (int) sizeof(u32));
    printf("%d\n", (int) sizeof(i32));

}
