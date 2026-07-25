/**
 * @file calibrate_dc6.c
 * @brief Calibrate `hamming_ot` and `hamming_ot_long` tests using Monte-Carlo
 * approach. It includes computation of mean, standard deviation and check
 * for normality using Lilliefors criterion.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
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



static int (*printf_mthr)(const char *format, ...);


static int printf_mute(const char *format, ...)
{
    (void) format;
    return 0;
}


typedef TestResults (*TestFuncWrap)(GeneratorState *, const void *);

typedef struct {
    TestFuncWrap test_func;
    void *opts;    
} HwTestInfo;

typedef struct {
    double *p; ///< p-value computed by the test itself
    double *z; ///< Empirical values from the te
} TestResultsArray;

static void TestResultsArray_init(TestResultsArray *obj, size_t len)
{
    obj->p = calloc(len, sizeof(double));
    obj->z = calloc(len, sizeof(double));
}

static void TestResultsArray_destruct(TestResultsArray *obj)
{
    free(obj->p); obj->p = NULL;
    free(obj->z); obj->z = NULL;
}

typedef struct {
    size_t init_pos;
    size_t nsamples;
    TestResultsArray res_ary;
    double *z_ary;
    GeneratorInfo *gi;
    CallerAPI *intf;
    HwTestInfo test_info;
    int nthreads;
} HammingThreadState;


static unsigned int get_nthreads(void)
{
    unsigned int nthreads = get_cpu_numcores();
    if (nthreads > 2) {
        nthreads--;
    }
    return nthreads;
}


static ThreadRetVal THREADFUNC_SPEC hamming_ot_run_test(void *udata)
{
    HammingThreadState *obj = udata;
    GeneratorState gen = GeneratorState_create(obj->gi, obj->intf);
    const size_t nthreads = (size_t) obj->nthreads;
    for (size_t i = obj->init_pos; i < obj->nsamples; i += nthreads) {
        printf_mthr("%lu of %lu\n",
            (unsigned long) (i + 1), (unsigned long) obj->nsamples);
        const TestResults res = obj->test_info.test_func(&gen, obj->test_info.opts);
        obj->res_ary.z[i] = res.x;
        obj->res_ary.p[i] = res.p;
    }
    GeneratorState_destruct(&gen);
    return 0;
}


TestResultsArray generate_sample(GeneratorInfo *gi, unsigned int nsamples, const HwTestInfo *test_info)
{
    CallerAPI intf = CallerAPI_init_mthr();
    GeneratorState gen = GeneratorState_create(gi, &intf);
    (void) test_info->test_func(&gen, test_info->opts);    
    GeneratorState_destruct(&gen);
    printf_mthr = intf.printf;
    intf.printf = printf_mute;
    const unsigned int nthreads = get_nthreads();
    printf("Number of threads: %u\n", nthreads);
    TestResultsArray res_ary;
    TestResultsArray_init(&res_ary, nsamples);
    HammingThreadState *threads = calloc(nthreads, sizeof(HammingThreadState));
    if (res_ary.z == NULL || res_ary.p == NULL || threads == NULL) {
        fprintf(stderr, "***** generate_sample: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    for (unsigned int i = 0; i < nthreads; i++) {
        threads[i].init_pos = i;
        threads[i].nsamples = nsamples;
        threads[i].res_ary = res_ary;
        threads[i].gi = gi;
        threads[i].intf = &intf;
        threads[i].test_info = *test_info;
        threads[i].nthreads = (int) nthreads;
    }
    init_thread_dispatcher();
    ThreadObj *handles = calloc(nthreads, sizeof(ThreadObj));
    if (handles == NULL) {
        fprintf(stderr, "***** generate_sample: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    for (unsigned int i = 0; i < nthreads; i++) {
        handles[i] = ThreadObj_create(hamming_ot_run_test, &threads[i], i + 1);
    }
    // Get data from threads
    for (unsigned int i = 0; i < nthreads; i++) {
        ThreadObj_wait(&handles[i]);
    }
    // Deallocate array
    free(handles);
    free(threads);
    CallerAPI_free();
    return res_ary;
}

HwTestInfo get_test_info(const char *name)
{                                   
    HwTestInfo out = {NULL, NULL};
    if (!strcmp(name, "w8_10m")) {  // 10^7 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_BYTES, .nbytes = 10000000};        
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w8_100m")) { // 10^8 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_BYTES, .nbytes = 100000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w8_1000m")) { // 10^9 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_BYTES, .nbytes = 1000000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;
    } else if (!strcmp(name, "w8_10000m")) { // 10^10 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_BYTES, .nbytes = 10000000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "wv_10m")) { // 10^7 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_VALUES, .nbytes = 10000000};        
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "wv_100m")) { // 10^8 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_VALUES, .nbytes = 100000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "wv_1000m")) { // 10^9 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_VALUES, .nbytes = 1000000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "wv_10000m")) { // 10^10 bytes
        static HammingOtOptions opts = {.mode = HAMMING_OT_VALUES, .nbytes = 10000000000};
        out.test_func = hamming_ot_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_10m")) { // 10^7 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 10000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_100m")) { // 10^8 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 100000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_1000m")) { // 10^9 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 1000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_10000m")) { // 10^10 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 10000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w256_10m")) { // 10^7 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 10000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w256_100m")) { // 10^8 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W256, .nvalues = 100000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;
    } else if (!strcmp(name, "w256_1000m")) { // 10^9 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W256, .nvalues = 1000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;
    } else if (!strcmp(name, "w256_10000m")) { // 10^10 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W256, .nvalues = 10000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;
    } else if (!strcmp(name, "w512_10m")) { // 10^7 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W512, .nvalues = 10000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;
    } else if (!strcmp(name, "w512_100m")) { // 10^8 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W512, .nvalues = 100000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w512_1000m")) { // 10^9 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W512, .nvalues = 1000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w512_10000m")) { // 10^10 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W512, .nvalues = 10000000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    }
    return out;
}


static void print_help(void)
{
    printf(
    "hamming_ot tests calibration\n"
    "Usage:\n"
    "  calibrate_dc6 subtest nbits nsamples\n"
    "  subtest: w8_10m, w8_100m, w8_1000m,\n"
    "           wv_10m, wv_100m, wv_1000m,\n"
    "           w128_10m, w128_100m, w128_1000m,\n"
    "           w256_10m, w256_100m, w256_1000m,\n"
    "           w512_10m, w512_100m, w512_1000m,\n"
    "  nbits:   32, 64 (in the generator output)\n"
    "  nsamples: number of samples (default is 10000)\n");

}


static int cmp_doubles(const void *aptr, const void *bptr)
{
    const double a = *((const double *) aptr);
    const double b = *((const double *) bptr);
    if (a < b) return -1;
    else if (a == b) return 0;
    else return 1;
}

static double calc_mean(const double *x, size_t len)
{
    double mean = 0.0;
    for (size_t i = 0; i < len; i++) {
        mean += x[i];
    }
    mean /= (double) len;
    return mean;
}

static double calc_std(const double *x, size_t len)
{
    const double mean = calc_mean(x, len);
    double std = 0.0;
    for (size_t i = 0; i < len; i++) {
        const double dx = x[i] - mean;
        std += dx * dx;
    }
    std = sqrt(std / (double) (len - 1));
    return std;
}


static void calc_statistics(double *x, size_t len)
{
    const double mean = calc_mean(x, len);
    const double std = calc_std(x, len);
    printf("mean: %g; std: %g\n", mean, std);
    // Renorm sample for Lilliefors test
    for (size_t i = 0; i < len; i++) {
        x[i] = (x[i] - mean) / std;
    }
    // Lilliefors test
    qsort(x, len, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < len; i++) {
        const double f = sr_stdnorm_cdf(x[i]);
        const double idbl = (double) i;
        const double Dplus = (idbl + 1.0) / (double) len - f;
        const double Dminus = f - idbl / (double) len;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    const double sqrt_len = sqrt((double) len);
    const double K = sqrt_len * D, Kcrit = 0.886;
    printf("Demp = %g; Kemp = %g\n", D, K);
    printf("Dcrit = %g; Kcrit = %g\n", Kcrit / sqrt_len, Kcrit);
}


static void calc_pvalue_statistics(const double *p, size_t len)
{
    const double mean = calc_mean(p, len);
    const double std = calc_std(p, len);
    printf("p-value statistics\n");
    printf("  mean: %g; std: %g\n", mean, std);
    printf("  std(theor): %g\n", sqrt(1.0 / 12.0));
}


int main(int argc, char *argv[])
{
    const char *name, *mod_name;
    char filename[64];
    unsigned int nbits = 32, nsamples = 10000;
    if (argc < 2) {
        print_help();
        return 0;
    }
    name = argv[1];
    if (argc >= 3) {
        nbits = (unsigned int) atoi(argv[2]);
        if (nbits != 32 && nbits != 64) {
            fprintf(stderr, "nbits: invalid value\n");
            return 1;
        }
    }
    if (argc >= 4) {
        nsamples = (unsigned int) atoi(argv[3]);
        if (nsamples < 10 || nsamples > 10000000) {
            fprintf(stderr, "nsamples: invalid value\n");
            return 1;
        }
    }

    HwTestInfo test_info = get_test_info(name);
    if (test_info.test_func == NULL) {
        fprintf(stderr, "subtest '%s' is not supported\n", name);
        return 1;
    }
    CallerAPI intf = CallerAPI_init_mthr();
    if (nbits == 32) {
        set_cmd_param("avx2");
        mod_name = "generators/chacha.dll";
    } else {
        set_cmd_param("aesni");
        mod_name = "generators/aes128.dll";
    }
    GeneratorModule mod = GeneratorModule_load(mod_name, &intf);
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }
    GeneratorInfo *gi = &mod.gen;
    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);
    snprintf(filename, 64, "hw_%s_%u.bin", name, nsamples);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open output file '%s'\n", filename);
        return 1;
    }
    const time_t tic = time(NULL);
    TestResultsArray res_ary = generate_sample(gi, nsamples, &test_info);
    unsigned long nseconds_total = (unsigned long) (time(NULL) - tic);
    printf("Elapsed time: "); print_elapsed_time(nseconds_total); printf("\n");


    fwrite(res_ary.z, nsamples, sizeof(double), fp);
    fwrite(res_ary.p, nsamples, sizeof(double), fp);
    fclose(fp);
    calc_statistics(res_ary.z, nsamples);
    calc_pvalue_statistics(res_ary.p, nsamples);
    TestResultsArray_destruct(&res_ary);
    GeneratorModule_unload(&mod);
    return 0;
}
