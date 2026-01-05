/**
 * @file sr_speed.c
 * @brief SmokeRand command line interface for performance benchmarks
 * for multiple generators. Automatically runs the `speed` battery
 * for each generator and makes tables with comparisons.
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include "smokerand/threads_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <float.h>

typedef struct {
    GeneratorModule mod;
    SpeedBatteryResults res;
} GeneratorModuleResults;


typedef enum {
    SPEED_UINT_CALL,
    SPEED_SUM_INLINE,
    SPEED_MEAN
} SpeedResultsModule;


static double
GeneratorModuleResults_speed(const GeneratorModuleResults *obj, SpeedResultsModule mode)
{    
    switch (mode) {
    case SPEED_UINT_CALL:
        return nbytes_to_mib(obj->res.uint.full.bytes_per_sec);
    case SPEED_SUM_INLINE:
        return nbytes_to_mib(obj->res.sum.full.bytes_per_sec);
    case SPEED_MEAN:
        return nbytes_to_mib(obj->res.mean.corr.bytes_per_sec);
    default:
        return NAN;
    }
}


static int GeneratorModuleResults_comp(const void *a, const void *b)
{
    const GeneratorModuleResults *obj_a = a, *obj_b = b;
    const double speed_a = obj_a->res.mean.corr.bytes_per_sec;
    const double speed_b = obj_b->res.mean.corr.bytes_per_sec;
    if (speed_a < speed_b) return -1;   
    else if (speed_a > speed_b) return 1;
    else return 0;
}


static void print_results(const GeneratorModuleResults *res, size_t len, int is_mean)
{
    // Header
    printf("%18s %8s ", "", "MiB/s");
    for (size_t i = 0; i < len; i++) {
        printf("%15.15s ", res[i].mod.gen.name);
    }
    printf("\n");
    // Table body
    for (size_t i = 0; i < len; i++) {
        const double speed_i = GeneratorModuleResults_speed(res + i, is_mean);
        printf("%18.18s %8.0f ", res[i].mod.gen.name, speed_i);
        for (size_t j = 0; j < len; j++) {
            const double speed_j = GeneratorModuleResults_speed(res + j, is_mean);
            const double ratio = 100.0 * (speed_i / speed_j - 1.0);
            printf("%14.0f%% ", ratio);
        }
        printf("\n");
    }
}


int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf(
            "SmokeRand: PRNG speed comparison\n"
            "Usage:\n"
            "  sr_speed gen1 gen2 ... genn\n");
        return 1;
    }

    CallerAPI intf = CallerAPI_init();
    // Load modules
    size_t nmodules_total = (size_t) (argc - 1);
    GeneratorModuleResults *res = calloc(nmodules_total, sizeof(GeneratorModuleResults));
    for (size_t i = 0; i < nmodules_total; i++) {
        res[i].mod = GeneratorModule_load(argv[i + 1], &intf);
        if (!res[i].mod.valid) {
            for (size_t j = 0; j < i; j++) {
                GeneratorModule_unload(&res[i].mod);
            }
            free(res);
            CallerAPI_free();
            return BATTERY_ERROR;
        }
    }
    // Speed test run
    for (size_t i = 0; i < nmodules_total; i++) {
        GeneratorInfo *gi = &res[i].mod.gen;
        printf("Running speed test for generator %s\n", gi->name);
        res[i].res = SpeedBatteryResults_get(gi, &intf);        
    }
    qsort(res, nmodules_total, sizeof(GeneratorModuleResults), GeneratorModuleResults_comp);
    // Show the report
    printf("-- Results for function calls: no baseline subtraction.\n");
    print_results(res, nmodules_total, SPEED_UINT_CALL);
    printf("-- Results for summation inside the cycle: no baseline subtraction.\n");
    print_results(res, nmodules_total, SPEED_SUM_INLINE);
    printf("-- Mean results for different modes: with baseline subtraction.\n");
    print_results(res, nmodules_total, SPEED_MEAN);
    // Unload modules
    for (size_t i = 0; i < nmodules_total; i++) {
        GeneratorModule_unload(&res[i].mod);
    }
    free(res);
    CallerAPI_free();
    return 0;
}
