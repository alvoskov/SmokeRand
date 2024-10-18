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



static int cmp_ints(const void *aptr, const void *bptr)
{
    uint64_t aval = *((uint64_t *) aptr), bval = *((uint64_t *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}



typedef struct {
    size_t n; ///< Number of values
    unsigned int e; ///< Leave only values with zeros in lower (e - 1) bits
} BirthdayOptions;


static double freq_to_chi2emp(unsigned long long *freq, size_t len)
{
    unsigned long long sum = 0, Ei;
    double chi2emp = 0.0;
    for (size_t i = 0; i < len; i++) {
        sum += freq[i];
    }
    Ei = sum / len;    
    for (size_t i = 0; i < len; i++) {
        chi2emp += pow((long long) freq[i] - (long long) Ei, 2.0) / Ei;
    }
    return chi2emp;
}

static inline void calc_frequencies(unsigned long long *bytefreq,
    unsigned long long *w16freq, const uint64_t u)
{
     uint64_t t = u;
     for (size_t j = 0; j < 8; j++) {
         bytefreq[t & 0xFF]++;
         t >>= 8;
     }
     t = u;
     for (size_t j = 0; j < 4; j++) {
         w16freq[t & 0xFFFF]++;
         t >>= 16;
     }
}

TestResults birthday_test(GeneratorState *obj, const BirthdayOptions *opts)
{
    TestResults ans = {.x = NAN, .p = NAN, .alpha = NAN};
    double lambda = pow(opts->n, 2.0) / pow(2.0, obj->gi->nbits - opts->e + 1.0);
    unsigned long long bytefreq[256], *w16freq;
    obj->intf->printf("  lambda = %g\n", lambda);
    obj->intf->printf("  Filling the array with 'birthdays'\n");
    uint64_t mask = (1ull << opts->e) - 1;
    time_t tic = time(NULL);
    uint64_t *x = calloc(opts->n, sizeof(uint64_t));
    if (x == NULL) {
        obj->intf->printf("  Not enough memory (2^%.0f bytes is required)\n",
            log2(opts->n * 8.0));
        return ans;
    }
    w16freq = calloc(65536, sizeof(unsigned long long));
    for (size_t i = 0; i < 256; i++) {
        bytefreq[i] = 0;
    }
    for (size_t i = 0; i < opts->n; i++) {
        uint64_t u;
        do {            
            u = obj->gi->get_bits(obj->state);
            calc_frequencies(bytefreq, w16freq, u);
        } while ((u & mask) != 0);
        x[i] = u;
        if (i % (opts->n / 1000) == 0) {
            unsigned long nseconds_total, nseconds_left;            
            nseconds_total = time(NULL) - tic;
            nseconds_left = ((unsigned long long) nseconds_total * (opts->n - i)) / (i + 1);
            obj->intf->printf("\r    %.1f %% completed; ", 100.0 * i / (double) opts->n);
            obj->intf->printf("time elapsed: "); print_elapsed_time(nseconds_total);
            obj->intf->printf("; time left: ");
            print_elapsed_time(nseconds_left);
            obj->intf->printf("    ");
        }
    }
    // Frequencies analysis
    double chi2emp_bytes = freq_to_chi2emp(bytefreq, 256);
    double chi2emp_w16 = freq_to_chi2emp(w16freq, 65536);
    printf("\n  chi2emp_bytes = %g, p = %g\n",
        chi2emp_bytes, chi2_cdf(chi2emp_bytes, 255));
    printf("  chi2emp_w16   = %g, p = %g\n",
        chi2emp_w16, chi2_cdf(chi2emp_w16, 65535));
    obj->intf->printf("\n");
    obj->intf->printf("\n  Sorting the array\n");
    // qsort is used instead of radix sort to prevent "out of memory" error:
    // 2^30 of u64 is 8GiB of data
    qsort(x, opts->n, sizeof(uint64_t), cmp_ints); // Not radix: to prevent "out of memory"
    obj->intf->printf("  Searching duplicates\n");
    unsigned long ndups = 0;
    for (size_t i = 0; i < opts->n - 1; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    ans.x = (double) ndups;
    ans.p = poisson_cdf(ans.x, lambda);
    ans.alpha = poisson_pvalue(ans.x, lambda);
    obj->intf->printf("  x = %g (ndups); p = %g; 1-p=%g\n", ans.x, ans.p, ans.alpha);
    free(x);
    free(w16freq);
    return ans;
}

void battery_birthday(GeneratorInfo *gen, const CallerAPI *intf)
{
    size_t n = 1ull << 30;
    BirthdayOptions opts_small = {.n = n, .e = 7};
    BirthdayOptions opts_large = {.n = n, .e = 10};
    intf->printf("64-bit birthday test\n");
    void *state = gen->create(intf);
    GeneratorState obj = {.gi = gen, .state = state, .intf = intf};
    TestResults ans = birthday_test(&obj, &opts_small);
    if (ans.x == 0) {
        intf->printf("  No duplicates found: more sensitive test required\n");
        intf->printf("  Running the variant with larger lambda\n");
        ans = birthday_test(&obj, &opts_large);
    }
    (void) ans;
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
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        return 0;        
    }

    
    GeneratorModule_unload(&mod);
    return 0;
}
           