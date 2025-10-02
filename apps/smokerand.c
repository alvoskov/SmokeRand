/**
 * @file smokerand.c
 * @brief SmokeRand command line interface.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include "smokerand/fileio.h"
#include "smokerand/threads_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <float.h>

void print_help(void)
{
    static const char help_str[] = 
    "SmokeRand: a test suite for pseudorandom number generators\n"
    "(C) 2024-2025 Alexey L. Voskov\n\n"
    "Usage: smokerand battery generator_lib [keys]\n"
    "battery: battery name; supported batteries:\n"
    "  General purpose batteries\n"
    "  - express   Express battery (32-64 MiB of data)\n"
    "  - brief     Fast battery (64-128 GiB of data)\n"
    "  - default   Slower but more sensitive battery (128-256 GiB of data)\n"
    "  - full      The slowest battery (1-2 TiB of data)\n"
    "  Special batteries\n"
    "  - birthday  64-bit birthday paradox based test.\n"
    "  - ising     Ising model based tests: Wolff and Metropolis algorithms.\n"
    "  - freq      8-bit and 16-bit words frequency adaptive tests.\n"  
    "  - @filename Load a custom battery from the file.\n"
    "  Special modes\n"
    "  - help      Print a built-in PRNG help (if available).\n"
    "  - selftest  Runs PRNG internal self-test (if available).\n"
    "  - speed     Measure speed of the generator\n"
    "  - stdout    Sends PRNG output to stdout in the binary form.\n"
    "generator_lib: name of dynamic library with PRNG or special mode name.\n"
    "  Special modes names:\n"
    "  - stdin32, stdin64  Get random sequence from stdin\n"
    "  - list              Print list of tests in the battery\n"
    "Optional keys\n"
    "  --filter=name Apply pre-defined filter to the generator output\n"
    "    reverse-bits   Reverse bits in the generator output\n"
    "    interleaved32  Process 64-bit generator output as interleaving 32-bit words\n"
    "    high32, low32  Analyse higher/lower 32 bits of 64-bit generator\n"
    "  --report-brief Show only failures in the report\n"
    "  --nthreads  Run battery in multithreaded mode (default number of threads)\n"
    "  --threads=n Run battery in multithreaded mode using n threads\n"
    "\n";

    printf(help_str);
}


typedef enum {
    FILTER_NONE,
    FILTER_REVERSE_BITS,
    FILTER_INTERLEAVED32,
    FILTER_HIGH32,
    FILTER_LOW32,
    FILTER_UNKNOWN
} GeneratorFilter;


GeneratorFilter GeneratorFilter_from_name(const char *name)
{
    if (!strcmp("reverse-bits", name)) {
        return FILTER_REVERSE_BITS;
    } else if (!strcmp("interleaved32", name)) {
        return FILTER_INTERLEAVED32;
    } else if (!strcmp("high32", name)) {
        return FILTER_HIGH32;
    } else if (!strcmp("low32", name)) {
        return FILTER_LOW32;
    } else {
        return FILTER_UNKNOWN;
    }
}

typedef struct {
    int nthreads;
    int testid;
    int maxlen_log2; ///< log2(len) for stdout length in bytes
    GeneratorFilter filter;
    ReportType report_type;
} SmokeRandSettings;

/**
 * @brief Returns a default number of threads suitable for the current
 * hardware configuration.
 * @details General principles:
 *
 * 1) Maximal number of threads is the number of CPU cores.
 * 2) For 32-bit systems - not more than 2 threads.
 * 3) If number of cores is more than 4 then leave one unloaded core.
 */
static int get_default_nthreads(int *ncores)
{
    int nthreads = get_cpu_numcores();
    if (ncores != NULL) {
        *ncores = nthreads;
    }
    if (sizeof(size_t) == 4 * sizeof(char) && nthreads > 2) {
        nthreads = 2;
    }
    if (nthreads > 4)
        nthreads--;
    return nthreads;
}

/**
 * @brief Process command line arguments to extract settings.
 * @param[out] obj  Output buffer for parsed settings.
 * @param[in]  argc Number of command line arguments (from `main` function).
 * @param[in]  argv Values of command line arguments (from `main` function).
 */
int SmokeRandSettings_load(SmokeRandSettings *obj, int argc, char *argv[])
{
    obj->nthreads = 1;
    obj->testid = TESTS_ALL;
    obj->filter = FILTER_NONE;
    obj->report_type = REPORT_FULL;
    obj->maxlen_log2 = 0;
    for (int i = 3; i < argc; i++) {
        char argname[32];
        int argval;
        char *eqpos = strchr(argv[i], '=');
        size_t len = strlen(argv[i]);
        if (!strcmp(argv[i], "--threads")) {
            int ncores;
            obj->nthreads = get_default_nthreads(&ncores);
            fprintf(stderr, "%d CPU cores detected\n", ncores);
            fprintf(stderr, "%d threads would be created\n", obj->nthreads);
            continue;
        }
        if (!strcmp(argv[i], "--report-brief")) {
            obj->report_type = REPORT_BRIEF;
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
            if (obj->filter == FILTER_UNKNOWN) {
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
        } else if (!strcmp(argname, "maxlen_log2")) {
            if (argval < 12 || argval > 63) {
                fprintf(stderr, "Invalid value of argument '%s'\n", argname);
                return 1;
            }
            obj->maxlen_log2 = argval;
        } else {
            fprintf(stderr, "Unknown argument '%s'\n", argname);
            return 1;
        }
    }
    return 0;
}


/**
 * @brief Run a battery of statistical test for a given generator.
 * @param battery_name  Name of tests battery: `default`, `express`, `brief`,
 * `full`, `help`, `selftest`, `speed`, `stdout`, `freq`, 'birthday`, `ising`.
 * @param gi   PRNG to be tested.
 * @param intf Pointers to API functions.
 * @param opts Pre-parsed command line options.
 */
int run_battery(const char *battery_name, GeneratorInfo *gi,
    CallerAPI *intf, SmokeRandSettings *opts)
{
    if (!strcmp(battery_name, "default")) {
        battery_default(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "express")) {
        battery_express(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "help")) {
        if (gi->description != NULL) {
            printf("%s\n", gi->description);
        } else {
            printf("Built-in help not found\n");
        }
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(gi, intf);
    } else if (!strcmp(battery_name, "speed")) {
        battery_speed(gi, intf);
    } else if (!strcmp(battery_name, "stdout")) {
        GeneratorInfo_bits_to_file(gi, intf, opts->maxlen_log2);
    } else if (!strcmp(battery_name, "freq")) {
        battery_blockfreq(gi, intf);
    } else if (!strcmp(battery_name, "birthday")) {
        battery_birthday(gi, intf);
    } else if (!strcmp(battery_name, "ising")) {
        battery_ising(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "unitsphere")) {
        battery_unit_sphere_volume(gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else if (!strcmp(battery_name, "dummy")) {
        fprintf(stderr, "Battery 'dummy': do nothing\n");
    } else if (battery_name[0] == '@') {
        if (battery_name[1] == '\0') {
            fprintf(stderr, "File name cannot be empty");
            return 1;
        }
        const char *filename = battery_name + 1;
        return battery_file(filename, gi, intf, opts->testid, opts->nthreads, opts->report_type);
    } else {
        fprintf(stderr, "Unknown battery %s\n", battery_name);
        return 1;
    }
    return 0;
}

int print_battery_info(const char *battery_name)
{
    if (!strcmp(battery_name, "express")) {
        battery_express(NULL, NULL, 0, 0, REPORT_FULL);
    } else if (!strcmp(battery_name, "default")) {
        battery_default(NULL, NULL, 0, 0, REPORT_FULL);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(NULL, NULL, 0, 0, REPORT_FULL);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(NULL, NULL, 0, 0, REPORT_FULL);
    } else if (battery_name[0] == '@') {
        battery_file(battery_name + 1, NULL, NULL, 0, 0, REPORT_FULL);
    } else {
        fprintf(stderr, "Information about battery %s is absent\n",
            battery_name);
        return 1;
    }
    return 0;
}


static void apply_filter(GeneratorInfo **gi, GeneratorFilter filter, GeneratorInfo *filter_gen)
{
    switch (filter) {
    case FILTER_REVERSE_BITS:
        *filter_gen = define_reversed_generator(*gi);
        *gi = filter_gen;
        fprintf(stderr, "All tests will be run with the reverse bits order\n");
        break;

    case FILTER_INTERLEAVED32:
        *filter_gen = define_interleaved_generator(*gi);
        *gi = filter_gen;
        fprintf(stderr, "All tests will be run with the interleaved\n");
        break;

    case FILTER_HIGH32:
        *filter_gen = define_high32_generator(*gi);
        *gi = filter_gen;
        fprintf(stderr, "All tests will be applied to the higher 32 bits only\n");
        break;

    case FILTER_LOW32:
        *filter_gen = define_low32_generator(*gi);
        *gi = filter_gen;
        fprintf(stderr, "All tests will be applied to the lower 32 bits only\n");
        break;

    case FILTER_NONE:
    case FILTER_UNKNOWN:
        break;
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

    if (!entfuncs_test()) {
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
        CallerAPI intf = (opts.nthreads == 1) ? CallerAPI_init() : CallerAPI_init_mthr();
        GeneratorModule mod = GeneratorModule_load(generator_lib, &intf);
        if (!mod.valid) {
            CallerAPI_free();
            return 1;
        }
        GeneratorInfo *gi = &mod.gen;
        if (gi->nbits != 64 && (opts.filter == FILTER_INTERLEAVED32 ||
            opts.filter == FILTER_HIGH32 || opts.filter == FILTER_LOW32)) {
            fprintf(stderr, "This filter is supported only for 64-bit generators\n");
            CallerAPI_free();
            return 1;
        }
        apply_filter(&gi, opts.filter, &filter_gen);
        GeneratorInfo_print(gi, is_stdout);
        int ans = run_battery(battery_name, gi, &intf, &opts);
        GeneratorModule_unload(&mod);
        CallerAPI_free();
        return ans;
    }
}
