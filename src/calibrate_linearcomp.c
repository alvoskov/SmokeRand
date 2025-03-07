#include "smokerand_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


unsigned int *calc_linearcomp_vector(GeneratorState *obj, size_t nvalues)
{
    static const LinearCompOptions lc_opts =
        {.nbits = 1000, .bitpos = linearcomp_bitpos_low};
    unsigned int *lc_vec = calloc(nvalues, sizeof(unsigned int));
    clock_t tic = clock(), toc;
    for (size_t i = 0; i < nvalues; i++) {
        TestResults res = linearcomp_test(obj, &lc_opts);
        if (i % 100 == 0) {
            toc = clock();
            if (toc - tic > CLOCKS_PER_SEC) {
                printf("%ld of %ld: %g\n", (long) (i + 1), (long) nvalues, res.x);
                tic = toc;
            }
        }
        lc_vec[i] = (unsigned int) res.x;
    }
    return lc_vec;
}

static int printf_null(const char *format, ...)
{
    (void) format;
    return 0;
}


int main()
{
    CallerAPI intf = CallerAPI_init();
    intf.printf = printf_null;
    GeneratorModule mod = GeneratorModule_load("generators/libspeck128_avx_shared.dll");
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }
    GeneratorInfo *gi = &mod.gen;
    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);
    // Calculate a vector of linear complexities
    size_t nvalues = 10000000;
    GeneratorState obj = GeneratorState_create(gi, &intf);
    unsigned int *lc_vec = calc_linearcomp_vector(&obj, nvalues);
    GeneratorState_free(&obj, &intf);
    GeneratorModule_unload(&mod);
    CallerAPI_free();
    // Write results to the file
    FILE *fp = fopen("linearcomp.m", "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open output file 'linearcomp.m'\n");
        return 1;
    }
    for (size_t i = 0; i < nvalues; i++) {
        if (i % 10 == 0) {
            fprintf(fp, "\n");
        }
        fprintf(fp, "%u, ", lc_vec[i]);
    }
    fclose(fp);
    free(lc_vec);
    return 0;
}
