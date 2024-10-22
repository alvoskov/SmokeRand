/**
 * @file smokerand.c
 * @brief SmokeRand command line interface.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"
#include "smokerand/bat_default.h"
#include "smokerand/bat_brief.h"
#include "smokerand/bat_full.h"
#include "smokerand/extratests.h"
#include <stdio.h>
#include <time.h>
#include <math.h>




/**
 * @brief Keeps PRNG speed measurements results.
 */
typedef struct
{
    double ns_per_call; ///< Nanoseconds per call
    double ticks_per_call; ///< Processor ticks per call
} SpeedResults;

/**
 * @brief PRNG speed measurement for uint32 output
 */
static SpeedResults measure_speed(GeneratorInfo *gen, const CallerAPI *intf)
{
    void *state = gen->create(intf);
    SpeedResults results;
    double ns_total = 0.0;
    for (size_t niter = 2; ns_total < 0.5e9; niter <<= 1) {
        clock_t tic = clock();
        uint64_t tic_proc = cpuclock();
        uint64_t sum = 0;
        for (size_t i = 0; i < niter; i++) {
            sum += gen->get_bits(state);
        }
        uint64_t toc_proc = cpuclock();
        clock_t toc = clock();
        ns_total = 1.0e9 * (toc - tic) / CLOCKS_PER_SEC;
        results.ns_per_call = ns_total / niter;
        results.ticks_per_call = (double) (toc_proc - tic_proc) / niter;
    }
    free(state);
    return results;
}

static void *dummy_create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

static uint64_t dummy_get_bits(void *state)
{
    (void) state;
    return 0;
}


void battery_speed(GeneratorInfo *gen, const CallerAPI *intf)
{
    GeneratorInfo dummy_gen = {.name = "dummy", .create = dummy_create,
        .get_bits = dummy_get_bits, .nbits = gen->nbits,
        .self_test = NULL};
    printf("===== Generator speed measurements =====\n");
    SpeedResults speed_full = measure_speed(gen, intf);
    SpeedResults speed_dummy = measure_speed(&dummy_gen, intf);
    double ns_per_call_corr = speed_full.ns_per_call - speed_dummy.ns_per_call;
    double ticks_per_call_corr = speed_full.ticks_per_call - speed_dummy.ticks_per_call;
    double cpb_corr = ticks_per_call_corr / (gen->nbits / 8);
    double gb_per_sec = (double) gen->nbits / 8.0 / (1.0e-9 * ns_per_call_corr) / pow(2.0, 30.0);

    printf("Generator name:    %s\n", gen->name);
    printf("Output size, bits: %d\n", (int) gen->nbits);
    printf("Nanoseconds per call:\n");
    printf("  Raw result:                %g\n", speed_full.ns_per_call);
    printf("  For empty 'dummy' PRNG:    %g\n", speed_dummy.ns_per_call);
    printf("  Corrected result:          %g\n", ns_per_call_corr);
    printf("  Corrected result (GB/sec): %g\n", gb_per_sec);
    printf("CPU ticks per call:\n");
    printf("  Raw result:                %g\n", speed_full.ticks_per_call);
    printf("  For empty 'dummy' PRNG:    %g\n", speed_dummy.ticks_per_call);
    printf("  Corrected result:          %g\n", ticks_per_call_corr);
    printf("  Corrected result (cpB):    %g\n\n", cpb_corr);
}





static inline void TestResults_print(const TestResults obj)
{
    printf("%s: p = %.3g; xemp = %g", obj.name, obj.p, obj.x);
    if (obj.p < 1e-10 || obj.p > 1 - 1e-10) {
        printf(" <<<<< FAIL\n");
    } else {
        printf("\n");
    }
}



void battery_self_test(GeneratorInfo *gen, const CallerAPI *intf)
{
    if (gen->self_test == 0) {
        intf->printf("Internal self-test not implemented\n");
        return;
    }
    intf->printf("Running internal self-test...\n");
    if (gen->self_test(intf)) {
        intf->printf("Internal self-test passed\n");
    } else {
        intf->printf("Internal self-test failed\n");
    }    
}

void print_help()
{
    printf("Usage: smokerand battery generator_lib\n");
    printf(" battery: battery name; supported batteries:\n");
    printf("   - default\n");
    printf("   - selftest\n");
    printf("  generator_lib: name of dynamic library with PRNG that export the functions:\n");
    printf("   - int gen_getinfo(GeneratorInfo *gi)\n");
    printf("\n");
}


typedef struct {
    int nthreads;
    int testid;
    int reverse_bits;
} SmokeRandSettings;


int SmokeRandSettings_load(SmokeRandSettings *obj, int argc, char *argv[])
{
    obj->nthreads = 1;
    obj->testid = TESTS_ALL;
    obj->reverse_bits = 0;
    for (int i = 3; i < argc; i++) {
        char argname[32];
        int argval;
        char *eqpos = strchr(argv[i], '=');
        size_t len = strlen(argv[i]);
        if (len < 3 || (argv[i][0] != '-' || argv[i][1] != '-') || eqpos == NULL) {
            printf("Argument '%s' should have --argname=argval layout\n", argv[i]);
            return 1;
        }
        size_t name_len = (eqpos - argv[i]) - 2;
        if (name_len >= 32) name_len = 31;
        memcpy(argname, &argv[i][2], name_len); argname[name_len] = '\0';
        argval = atoi(eqpos + 1);
        if (!strcmp(argname, "nthreads")) {
            obj->nthreads = argval;
            if (argval <= 0) {
                printf("Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else if (!strcmp(argname, "testid")) {
            obj->testid = argval;
            if (argval <= 0) {
                printf("Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else if (!strcmp(argname, "reverse-bits")) {
            obj->reverse_bits = argval;
        } else {
            printf("Unknown argument '%s'\n", argname);
            return 1;
        }
    }
    return 0;
}


int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        print_help();
        return 0;
    }
    GeneratorInfo reversed_gen;
    SmokeRandSettings opts;
    if (SmokeRandSettings_load(&opts, argc, argv)) {
        return 1;
    }
    CallerAPI intf = (opts.nthreads == 1) ? CallerAPI_init() : CallerAPI_init_mthr();
    char *battery_name = argv[1];
    char *generator_lib = argv[2];

    GeneratorModule mod = GeneratorModule_load(generator_lib);
    if (!mod.valid) {
        return 1;
    }

    printf("Seed generator self-test: %s\n",
        xxtea_test() ? "PASSED" : "FAILED");

    GeneratorInfo *gi = &mod.gen;
    if (opts.reverse_bits) {
        reversed_gen = reversed_generator_set(gi);
        gi = &reversed_gen;
        printf("All tests will be run with the reverse bits order\n");
    }

    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);


    if (!strcmp(battery_name, "default")) {
        battery_default(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(gi, &intf);
    } else if (!strcmp(battery_name, "speed")) {
        battery_speed(gi, &intf);
    } else if (!strcmp(battery_name, "birthday")) {
        battery_birthday(gi, &intf);
    } else if (!strcmp(battery_name, "ising")) {
        battery_ising(gi, &intf);
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        return 0;        
    }

    
    GeneratorModule_unload(&mod);
    return 0;
}
           