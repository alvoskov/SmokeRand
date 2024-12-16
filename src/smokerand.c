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
#include "smokerand/bat_dos16.h"
#include "smokerand/extratests.h"
#include "smokerand/fileio.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <float.h>

#define SUM_BLOCK_SIZE 32768

/**
 * @brief Keeps PRNG speed measurements results.
 */
typedef struct
{
    double ns_per_call; ///< Nanoseconds per call
    double ticks_per_call; ///< Processor ticks per call
} SpeedResults;

typedef enum {
    speed_uint, ///< Single value (emulates call of PRNG function)
    speed_sum   ///< Sum of values (emulates usage of inline PRNG)
} SpeedMeasurementMode;

/**
 * @brief PRNG speed measurement for uint output.
 * @details Two modes are supported: measurement of one function call
 * and measurement of sum computation by PRNG inside the cycle.
 */
static SpeedResults measure_speed(GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorState obj = GeneratorState_create(gen, intf);
    SpeedResults results;
    double ns_total = 0.0;
    for (size_t niter = 2; ns_total < 0.5e9; niter <<= 1) {
        clock_t tic = clock();
        uint64_t tic_proc = cpuclock();
        if (mode == speed_uint) {
            uint64_t sum = 0;
            for (size_t i = 0; i < niter; i++) {
                sum += obj.gi->get_bits(obj.state);
            }
            (void) sum;
        } else {
            uint64_t sum = 0;
            for (size_t i = 0; i < niter; i++) {
                sum += obj.gi->get_sum(obj.state, SUM_BLOCK_SIZE);
            }
            (void) sum;
        }
        uint64_t toc_proc = cpuclock();
        clock_t toc = clock();
        ns_total = 1.0e9 * (toc - tic) / CLOCKS_PER_SEC;
        results.ns_per_call = ns_total / niter;
        results.ticks_per_call = (double) (toc_proc - tic_proc) / niter;
    }
    GeneratorState_free(&obj, intf);
    return results;
}

static void *dummy_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) intf; (void) gi;
    return NULL;
}

static void dummy_free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) state; (void) gi; (void) intf;
}

static uint64_t dummy_get_bits(void *state)
{
    (void) state;
    return 0;
}

/**
 * @brief Sums some predefined elements in the cycle to measure
 * baseline time for PRNG performance estimation. It intentionally
 * uses a table with numbers to override optimization of compilers
 * (exclusion of cycle, even replacement of cycle for arithmetic
 * progression to formula etc.) Inspired by DOOM PRNG.
 */
static uint64_t dummy_get_sum(void *state, size_t len)
{
    uint64_t data[] = {9338, 34516, 60623, 45281,
        9064,   60090,  62764,  5557,
        44347,  35277,  25712,  20552,
        50645,  61072,  26719,  21307};
    uint64_t sum = 0;
    (void) state;
    for (size_t i = 0; i < len; i++) {
        sum += data[i & 0xF];
    }
    return sum;
}



void battery_speed_test(GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorInfo dummy_gen = {.name = "dummy", .description = "DUMMY",
        .create = dummy_create, .free = dummy_free,
        .get_bits = dummy_get_bits, .get_sum = dummy_get_sum,
        .self_test = NULL, .parent = NULL};
    dummy_gen.nbits = gen->nbits;
    SpeedResults speed_full = measure_speed(gen, intf, mode);
    SpeedResults speed_dummy = measure_speed(&dummy_gen, intf, mode);
    size_t block_size = (mode == speed_uint) ? 1 : SUM_BLOCK_SIZE;
    size_t nbytes = block_size * gen->nbits / 8;
    double ns_per_call_corr = speed_full.ns_per_call - speed_dummy.ns_per_call;
    double ticks_per_call_corr = speed_full.ticks_per_call - speed_dummy.ticks_per_call;
    double cpb_corr = ticks_per_call_corr / nbytes;
    double gb_per_sec = (double) nbytes / (1.0e-9 * ns_per_call_corr) / pow(2.0, 30.0);

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

void battery_speed(GeneratorInfo *gen, const CallerAPI *intf)
{
    printf("===== Generator speed measurements =====\n");
    printf("----- Speed test for uint generation -----\n");
    battery_speed_test(gen, intf, speed_uint);
    printf("----- Speed test for uint sum generation -----\n");
    if (gen->get_sum == NULL) {
        printf("  Not implemented\n");
    } else {
        battery_speed_test(gen, intf, speed_sum);
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

void print_help(void)
{
    static const char help_str[] = 
    "SmokeRand: a test suite for pseudorandom number generators\n"
    "(C) 2024 Alexey L. Voskov\n\n"
    "Usage: smokerand battery generator_lib [keys]\n"
    "battery: battery name; supported batteries:\n"
    "  General purpose batteries\n"
    "  - brief    Fast battery (64-128 GiB of data)\n"
    "  - default  Slower but more sensitive battery (128-256 GiB of data)\n"
    "  - full     The slowest battery (1-2 TiB of data)\n"
    "  - dos16    Express battery (32-64 MiB of data)\n"
    "  Special batteries\n"
    "  - birthday 64-bit birthday paradox based test.\n"
    "  - ising    Ising model based tests: Wolff and Metropolis algorithms.\n"
    "  - freq     8-bit and 16-bit words frequency adaptive tests.\n"
    "  Special modes\n"
    "  - selftest Runs PRNG internal self-test (if available).\n"
    "  - speed    Measure speed of the generator\n"
    "  - stdout   Sends PRNG output to stdout in the binary form.\n"
    "generator_lib: name of dynamic library with PRNG or special mode name.\n"
    "  Special modes names:\n"
    "  - stdin32, stdin64  Get random sequence from stdin\n"
    "  - list              Print list of tests in the battery\n"
    "Optional keys\n"
    "  --threads=n Run battery in multithreaded mode using n threads\n"
    "  --nthreads  Run battery in multithreaded mode (default number of threads\n"
    "  --filter=name Apply pre-defined filter to the generator output\n"
    "    reverse-bits   Reverse bits in the generator output\n"
    "    interleaved32  Process 64-bit generator output as interleaving 32-bit words\n"
    "    high32, low32  Analyse higher/lower 32 bits of 64-bit generator\n"
    "\n";

    printf(help_str);
}


typedef enum {
    filter_none,
    filter_reverse_bits,
    filter_interleaved32,
    filter_high32,
    filter_low32,
    filter_unknown
} GeneratorFilter;


GeneratorFilter GeneratorFilter_from_name(const char *name)
{
    if (!strcmp("reverse-bits", name)) {
        return filter_reverse_bits;
    } else if (!strcmp("interleaved32", name)) {
        return filter_interleaved32;
    } else if (!strcmp("high32", name)) {
        return filter_high32;
    } else if (!strcmp("low32", name)) {
        return filter_low32;
    } else {
        return filter_unknown;
    }
}

typedef struct {
    int nthreads;
    int testid;
    GeneratorFilter filter;
} SmokeRandSettings;


int SmokeRandSettings_load(SmokeRandSettings *obj, int argc, char *argv[])
{
    obj->nthreads = 1;
    obj->testid = TESTS_ALL;
    obj->filter = filter_none;
    for (int i = 3; i < argc; i++) {
        char argname[32];
        int argval;
        char *eqpos = strchr(argv[i], '=');
        size_t len = strlen(argv[i]);
        if (!strcmp(argv[i], "--threads")) {
            obj->nthreads = get_cpu_numcores();
            fprintf(stderr, "%d CPU cores detected\n", obj->nthreads);
            continue;
        }
        if (len < 3 || (argv[i][0] != '-' || argv[i][1] != '-') || eqpos == NULL) {
            fprintf(stderr, "Argument '%s' should have --argname=argval layout\n", argv[i]);
            return 1;
        }
        size_t name_len = (eqpos - argv[i]) - 2;
        if (name_len >= 32) name_len = 31;
        memcpy(argname, &argv[i][2], name_len); argname[name_len] = '\0';
        // Text arguments processing
        if (!strcmp(argname, "param")) {
            set_cmd_param(eqpos + 1);
            continue;
        } else if (!strcmp(argname, "filter")) {
            obj->filter = GeneratorFilter_from_name(eqpos + 1);
            if (obj->filter == filter_unknown) {
                fprintf(stderr, "Unknown filter %s\n", eqpos + 1);
                return 1;
            }
            continue;
        }
        // Numerical arguments processing
        argval = atoi(eqpos + 1);
        if (!strcmp(argname, "nthreads")) {
            obj->nthreads = argval;
            if (argval <= 0) {
                fprintf(stderr, "Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else if (!strcmp(argname, "testid")) {
            obj->testid = argval;
            if (argval <= 0) {
                fprintf(stderr, "Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else {
            fprintf(stderr, "Unknown argument '%s'\n", argname);
            return 1;
        }
    }
    return 0;
}


int run_battery(const char *battery_name, GeneratorInfo *gi,
    CallerAPI *intf, SmokeRandSettings *opts)
{
    if (!strcmp(battery_name, "default")) {
        battery_default(gi, intf, opts->testid, opts->nthreads);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(gi, intf, opts->testid, opts->nthreads);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(gi, intf, opts->testid, opts->nthreads);
    } else if (!strcmp(battery_name, "dos16")) {
        battery_dos16(gi, intf, opts->testid, opts->nthreads);
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(gi, intf);
    } else if (!strcmp(battery_name, "speed")) {
        battery_speed(gi, intf);
    } else if (!strcmp(battery_name, "stdout")) {
        GeneratorInfo_bits_to_file(gi, intf);
    } else if (!strcmp(battery_name, "freq")) {
        battery_blockfreq(gi, intf);
    } else if (!strcmp(battery_name, "birthday")) {
        battery_birthday(gi, intf);
    } else if (!strcmp(battery_name, "ising")) {
        battery_ising(gi, intf, opts->testid, opts->nthreads);
    } else {
        fprintf(stderr, "Unknown battery %s\n", battery_name);
        return 1;
    }
    return 0;
}

int print_battery_info(const char *battery_name)
{
    if (!strcmp(battery_name, "default")) {
        battery_default(NULL, NULL, 0, 0);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(NULL, NULL, 0, 0);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(NULL, NULL, 0, 0);
    } else {
        fprintf(stderr, "Information about battery %s is absent\n",
            battery_name);
        return 1;
    }
    return 0;
}

void GeneratorInfo_print(const GeneratorInfo *gi, int to_stderr)
{
    FILE *fp = (to_stderr) ? stderr : stdout;
    fprintf(fp, "Generator name:    %s\n", gi->name);
    fprintf(fp, "Output size, bits: %d\n", gi->nbits);
    if (gi->parent != NULL) {
        fprintf(fp, "Parent generator:\n");
        fprintf(fp, "  Name:              %s\n", gi->parent->name);
        fprintf(fp, "  Output size, bits: %d\n", gi->parent->nbits);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        print_help();
        return 0;
    }
    GeneratorInfo filter_gen;
    SmokeRandSettings opts;
    if (SmokeRandSettings_load(&opts, argc, argv)) {
        return 1;
    }
    char *battery_name = argv[1];
    char *generator_lib = argv[2];
    int is_stdin32 = !strcmp(generator_lib, "stdin32");
    int is_stdin64 = !strcmp(generator_lib, "stdin64");
    int is_stdout = !strcmp(battery_name, "stdout");
    set_use_stderr_for_printf(is_stdout); // Messages mustn't be in PRNG output

    if (opts.nthreads > 1 && (is_stdin32 || is_stdin64)) {
        fprintf(stderr, "Multithreading is not supported for stdin32/stdin64\n");
        return 1;
    }

    if (!xxtea_test()) {
        fprintf(stderr, "Seed generator self-test failed\n");
        return 1;
    }

    if (!strcmp(generator_lib, "list")) {
        return print_battery_info(battery_name);
    }

    if (is_stdin32 || is_stdin64) {
        CallerAPI intf = CallerAPI_init();
        GeneratorInfo stdin_gi;
        if (is_stdin32) {
            stdin_gi = StdinCollector_get_info(stdin_collector_32bit);
        } else {
            stdin_gi = StdinCollector_get_info(stdin_collector_64bit);
        }
        GeneratorInfo_print(&stdin_gi, is_stdout);
        int ans = run_battery(battery_name, &stdin_gi, &intf, &opts);
        StdinCollector_print_report();
        CallerAPI_free();
        return ans;
    } else {
        GeneratorModule mod = GeneratorModule_load(generator_lib);
        if (!mod.valid) {
            return 1;
        }
        GeneratorInfo *gi = &mod.gen;
        if (gi->nbits != 64 && (opts.filter == filter_interleaved32 ||
            opts.filter == filter_high32 || opts.filter == filter_low32)) {
            fprintf(stderr, "This filter is supported only for 64-bit generators\n");
            return 1;
        }
        switch (opts.filter) {
        case filter_reverse_bits:
            filter_gen = define_reversed_generator(gi);
            gi = &filter_gen;
            fprintf(stderr, "All tests will be run with the reverse bits order\n");
            break;

        case filter_interleaved32:
            filter_gen = define_interleaved_generator(gi);
            gi = &filter_gen;
            fprintf(stderr, "All tests will be run with the interleaved\n");
            break;

        case filter_high32:
            filter_gen = define_high32_generator(gi);
            gi = &filter_gen;
            fprintf(stderr, "All tests will be applied to the higher 32 bits only\n");
            break;

        case filter_low32:
            filter_gen = define_low32_generator(gi);
            gi = &filter_gen;
            fprintf(stderr, "All tests will be applied to the lower 32 bits only\n");
            break;

        case filter_none:
        case filter_unknown:
            break;
        }
        GeneratorInfo_print(gi, is_stdout);
        CallerAPI intf = (opts.nthreads == 1) ? CallerAPI_init() : CallerAPI_init_mthr();
        int ans = run_battery(battery_name, gi, &intf, &opts);
        GeneratorModule_unload(&mod);
        CallerAPI_free();
        return ans;
    }
}
