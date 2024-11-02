#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"
#include "smokerand/bat_default.h"
#include "smokerand/bat_brief.h"
#include "smokerand/bat_full.h"
#include "smokerand/extratests.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <float.h>

static TestResults hamming_dc6_all_test(GeneratorState *obj)
{
    HammingDc6Options opts = {.mode = hamming_dc6_bytes, .nbytes = 100000000};
//    HammingDc6Options opts = {.mode = hamming_dc6_values, .nbytes = 100000000};
    return hamming_dc6_test(obj, &opts);
}


int main()
{
    int nsamples = 32768;
    CallerAPI intf = CallerAPI_init();
    GeneratorModule mod = GeneratorModule_load("generators/libspeck128_avx_shared.dll");
//    GeneratorModule mod = GeneratorModule_load("generators/libchacha_avx_shared.dll");
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }
    GeneratorInfo *gi = &mod.gen;
    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);
    FILE *fp = fopen("dc6.m", "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open output file 'dc6.m'\n");
        return 1;
    }
    void *state = gi->create(&intf);
    double *z_ary = calloc(nsamples, sizeof(double));
    GeneratorState obj = {.gi = gi, .state = state, .intf = &intf};
    for (int i = 0; i < nsamples; i++) {
        printf("%d out of %d\n", i + 1, nsamples);
        TestResults res = hamming_dc6_all_test(&obj);
        z_ary[i] = res.x;
    }
    fprintf(fp, "z = [");
    for (int i = 0; i < nsamples; i++) {
        if (i != nsamples - 1) {
            fprintf(fp, "%g, ", z_ary[i]);
            if ((i + 1) % 8 == 0) {
                fprintf(fp, "\n");
            }
        } else {
            fprintf(fp, "%g", z_ary[i]);
        }
    }
    fprintf(fp, "]\n");
    fclose(fp);

    intf.free(obj.state);
    free(z_ary);
    GeneratorModule_unload(&mod);
    CallerAPI_free();
    return 0;
}
