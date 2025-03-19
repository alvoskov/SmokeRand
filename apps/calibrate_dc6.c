/**
 * @file calibrate_dc6.c
 * @brief Calibrate `hamming_ot` and `hamming_ot_long` tests using Monte-Carlo
 * approach. It includes computation of mean, standard deviation and check
 * for normality using Lilliefors criterion.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    size_t init_pos;
    size_t nsamples;    
    double *z_ary;
    GeneratorInfo *gi;
    CallerAPI *intf;
    HwTestInfo test_info;
    int nthreads;
} HammingThreadState;

static ThreadRetVal THREADFUNC_SPEC hamming_ot_run_test(void *udata)
{
    HammingThreadState *obj = udata;
    GeneratorState gen = GeneratorState_create(obj->gi, obj->intf);
    for (size_t i = obj->init_pos; i < obj->nsamples; i += obj->nthreads) {
        printf_mthr("%lu of %lu\n",
            (unsigned long) (i + 1), (unsigned long) obj->nsamples);
        TestResults res = obj->test_info.test_func(&gen, obj->test_info.opts);
        obj->z_ary[i] = res.x;
    }
    GeneratorState_free(&gen, obj->intf);
    return 0;
}


double *generate_sample(GeneratorInfo *gi, int nsamples, const HwTestInfo *test_info)
{
    CallerAPI intf = CallerAPI_init_mthr();
    GeneratorState gen = GeneratorState_create(gi, &intf);
    (void) test_info->test_func(&gen, test_info->opts);    
    GeneratorState_free(&gen, &intf);
    printf_mthr = intf.printf;
    intf.printf = printf_mute;
    int nthreads = get_cpu_numcores();
    printf("Number of threads: %d\n", nthreads);
    double *z_ary = calloc(nsamples, sizeof(double));
    HammingThreadState *threads = calloc(nthreads, sizeof(HammingThreadState));
    if (z_ary == NULL || threads == NULL) {
        fprintf(stderr, "***** generate_sample: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nthreads; i++) {
        threads[i].init_pos = i;
        threads[i].nsamples = nsamples;
        threads[i].z_ary = z_ary;
        threads[i].gi = gi;
        threads[i].intf = &intf;
        threads[i].test_info = *test_info;
        threads[i].nthreads = nthreads;
    }
    init_thread_dispatcher();
    ThreadObj *handles = calloc(sizeof(ThreadObj), nthreads);
    if (handles == NULL) {
        fprintf(stderr, "***** generate_sample: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nthreads; i++) {
        handles[i] = ThreadObj_create(hamming_ot_run_test, &threads[i], i + 1);
    }
    // Get data from threads
    for (int i = 0; i < nthreads; i++) {
        ThreadObj_wait(&handles[i]);
    }
    // Deallocate array
    free(handles);
    free(threads);
    CallerAPI_free();
    return z_ary;
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
    } else if (!strcmp(name, "w128_10m")) { // 10^7 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 10000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_100m")) { // 10^8 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 100000000};
        out.test_func = hamming_ot_long_test_wrap; out.opts = &opts;        
    } else if (!strcmp(name, "w128_1000m")) { // 10^9 values
        static HammingOtLongOptions opts = {.wordsize = HAMMING_OT_W128, .nvalues = 1000000000};
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
    "           w128_10m w128_100m, w128_100m\n"
    "  nbits:   32, 64 (in the generator output)\n"
    "  nsamples: number of samples (default is 10000)\n");

}


static int cmp_doubles(const void *aptr, const void *bptr)
{
    double aval = *((double *) aptr), bval = *((double *) bptr);
    if (aval < bval) return -1;
    else if (aval == bval) return 0;
    else return 1;
}


static void calc_statistics(double *x, size_t len)
{
    double mean = 0.0, std = 0.0;
    for (size_t i = 0; i < len; i++) {
        mean += x[i];
    }
    mean /= len;
    for (size_t i = 0; i < len; i++) {
        double dx = x[i] - mean;
        std += dx * dx;
    }
    std = sqrt(std / (len - 1));
    printf("mean: %g; std: %g\n", mean, std);
    // Renorm sample for Lilliefors test
    for (size_t i = 0; i < len; i++) {
        x[i] = (x[i] - mean) / std;
    }
    // Lilliefors test
    qsort(x, len, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < len; i++) {
        double f = sr_stdnorm_cdf(x[i]);
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / len - f;
        double Dminus = f - idbl / len;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double sqrt_len = sqrt((double) len);
    double K = sqrt_len * D, Kcrit = 0.886;
    printf("Demp = %g; Kemp = %g\n", D, K);
    printf("Dcrit = %g; Kcrit = %g\n", Kcrit / sqrt_len, Kcrit);
}


int main(int argc, char *argv[])
{
    const char *name, *mod_name;
    char filename[64];
    int nbits = 32, nsamples = 10000;
    if (argc < 2) {
        print_help();
        return 0;
    }
    name = argv[1];
    if (argc >= 3) {
        nbits = atoi(argv[2]);
        if (nbits != 32 && nbits != 64) {
            fprintf(stderr, "nbits: invalid value\n");
            return 1;
        }
    }
    if (argc >= 4) {
        nsamples = atoi(argv[3]);
        if (nsamples < 10 || nsamples > 10000000) {
            fprintf(stderr, "nsamples: invalid value\n");
            return 1;
        }
    }

    HwTestInfo test_info = get_test_info(name);
    mod_name = (nbits == 32) ? "generators/chacha_avx.dll" : "generators/speck128_avx.dll";
    GeneratorModule mod = GeneratorModule_load(mod_name);
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }
    GeneratorInfo *gi = &mod.gen;
    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);
    snprintf(filename, 64, "hw_%s_%d.bin", name, nsamples);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open output file '%s'\n", filename);
        return 1;
    }
    time_t tic = time(NULL);
    double *z_ary = generate_sample(gi, nsamples, &test_info);
    unsigned long nseconds_total = (unsigned long) (time(NULL) - tic);
    printf("Elapsed time: "); print_elapsed_time(nseconds_total); printf("\n");


    fwrite(z_ary, nsamples, sizeof(double), fp);
    fclose(fp);
    calc_statistics(z_ary, nsamples);
    free(z_ary);
    GeneratorModule_unload(&mod);
    return 0;
}
