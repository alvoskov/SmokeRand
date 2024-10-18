/**
 * @file core.c
 * @brief Subroutines and special functions required for implementation
 * of statistical tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/core.h"
#include "smokerand/entropy.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>



static Entropy entropy = {{0, 0, 0, 0}, 0};

static uint32_t get_seed32(void)
{
    return Entropy_seed64(&entropy) >> 32;
}

static uint64_t get_seed64(void)
{
    return Entropy_seed64(&entropy);
}

CallerAPI CallerAPI_init(void)
{
    CallerAPI intf;    
    if (entropy.state == 0) {
        Entropy_init(&entropy);
    }
    intf.get_seed32 = get_seed32;
    intf.get_seed64 = get_seed64;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf;
    intf.strcmp = strcmp;
    return intf;
}


static pthread_mutex_t get_seed32_mt_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t get_seed64_mt_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t get_seed32_mt(void)
{
    pthread_mutex_lock(&get_seed32_mt_mutex);
    uint32_t seed = Entropy_seed64(&entropy) >> 32;
    pthread_mutex_unlock(&get_seed32_mt_mutex);
    return seed;
}

static uint64_t get_seed64_mt(void)
{
    pthread_mutex_lock(&get_seed64_mt_mutex);
    uint64_t seed = Entropy_seed64(&entropy);
    pthread_mutex_unlock(&get_seed64_mt_mutex);
    return seed;
}


static pthread_mutex_t printf_mt_mutex = PTHREAD_MUTEX_INITIALIZER;

static int printf_mt(const char *format, ...)
{
    pthread_mutex_lock(&printf_mt_mutex);
    va_list args;
    va_start(args, format);
    printf("=== THREAD #%2d ===> ", (int) pthread_self());
    int ans = vprintf(format, args);
    va_end(args);
    pthread_mutex_unlock(&printf_mt_mutex);
    return ans;
}

/**
 * @brief Return thread-safe versions of functions
 */
CallerAPI CallerAPI_init_mthr(void)
{
    CallerAPI intf;    
    if (entropy.state == 0) {
        Entropy_init(&entropy);
    }
    intf.get_seed32 = get_seed32_mt;
    intf.get_seed64 = get_seed64_mt;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_mt;
    intf.strcmp = strcmp;
    return intf;
}



/////////////////////////////////////// 
///// Some mathematical functions /////
///////////////////////////////////////



PValueCategory get_pvalue_category(double pvalue)
{
    const double pvalue_fail_val = 1.0e-10;
    const double pvalue_warn_val = 1.0e-3;
    if (pvalue < pvalue_fail_val || pvalue > 1.0 - pvalue_fail_val) {
        return pvalue_failed;
    } else if (pvalue < pvalue_warn_val || pvalue > 1.0 - pvalue_warn_val) {
        return pvalue_warning;
    } else {
        return pvalue_passed;
    }
}

const char *interpret_pvalue(double pvalue)
{
    switch (get_pvalue_category(pvalue)) {
    case pvalue_failed:
        return "FAIL";
    case pvalue_warning:
        return "SUSPICIOUS";
    case pvalue_passed:
        return "Ok";
    default:
        return "???";
    }
}





/**
 * @brief A crude approximation for Kolmogorov-Smirnov test p-values.
 * @details Control (x, p) points from scipy.special.kolmogorov:
 * - (5.0, 3.8575e-22), (2.0, 0.0006709), (1.1, 0.1777),
 * - (1.0, 0.2700), (0.9, 0.3927)
 */
double ks_pvalue(double x)
{
    double xsq = x * x;
    if (x <= 0.0) {
        return 1.0;
    } else if (x > 1.0) {
        return 2.0 * (exp(-2.0 * xsq) - exp(-8.0 * xsq));
    } else {
        double t = -M_PI * M_PI / (8 * xsq);
        return 1.0 - sqrt(2 * M_PI) / x * (exp(t) + exp(9*t));
    }
}


static double gammainc_lower_series(double a, double x)
{
    double mul = exp(-x + a*log(x) - lgamma(a)), sum = 0.0;
    double t = 1.0 / a;
    for (int i = 1; i < 1000000 && t > DBL_EPSILON; i++) {
        sum += t;
        t *= x / (a + i);
    }
    return mul * sum;
}

static double gammainc_upper_contfrac(double a, double x)
{
    double mul = exp(-x + a*log(x) - lgamma(a));
    double b0 = x + 1 - a, b1 = x + 3 - a;
    double p0 = b0, q0 = 1.0;
    double p1 = b1 * b0 + (a - 1.0), q1 = b1;
    double f = p1/q1, f_old = p0/q0;
    double c = p1/p0, d = q0 / q1;
    for (int i = 2; i < 1000000 && fabs(f - f_old) > DBL_EPSILON; i++) {
        f_old = f;
        double bk = x + 2*i + 1 - a;
        double ak = i * (a - i);
        if (c < 1e-30) c = 1e-30;
        c = bk + ak / c;
        d = 1.0 / (bk + ak * d);
        f *= c * d;
    }
    return mul / f;
}

/**
 * @brief An approximation of lower incomplete gamma function for
 * implementation of Poisson C.D.F. and its p-values computation.
 * @details References:
 * - https://gmd.copernicus.org/articles/3/329/2010/gmd-3-329-2010.pdf
 * - https://functions.wolfram.com/GammaBetaErf/Gamma2/06/02/02/
 */
double gammainc(double a, double x)
{
    if (x < a + 1) {
        return gammainc_lower_series(a, x);
    } else {
        return 1.0 - gammainc_upper_contfrac(a, x);
    }
}

/**
 * @brief An approximation of upper incomplemete gamma function
 */
double gammainc_upper(double a, double x)
{
    if (x < a + 1) {
        return 1.0 - gammainc_lower_series(a, x);
    } else {
        return gammainc_upper_contfrac(a, x);
    }
}

/**
 * @brief Poisson distribution C.D.F. (cumulative distribution function)
 */
double poisson_cdf(double x, double lambda)
{
    return gammainc_upper(floor(x) + 1.0, lambda);
}

/**
 * @brief Calculate p-value for Poission distribution as 1 - F(x)
 * where F(x) is Poission distribution C.D.F.
 */
double poisson_pvalue(double x, double lambda)
{
    return gammainc(floor(x) + 1.0, lambda);
}


/**
 * @brief Asymptotic approximation for chi-square c.d.f.
 * @details References:
 * - Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double chi2_cdf(double x, unsigned long f)
{
    double s2 = 2.0 / (9.0 * f);
    double mu = 1 - s2;
    double z = (pow(x/f, 1.0/3.0) - mu) / sqrt(s2);
    return 0.5 + 0.5 * erf(z / sqrt(2));
}


/**
 * @brief Asymptotic approximation for p-value of chi-square distribution.
 * @details References:
 * - Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double chi2_pvalue(double x, unsigned long f)
{
    double s2 = 2.0 / (9.0 * f);
    double mu = 1 - s2;
    double z = (pow(x/f, 1.0/3.0) - mu) / sqrt(s2);
    return 0.5 * erfc(z / sqrt(2));
}

//////////////////////////////////////////////////
///// Subroutines for working with C modules /////
//////////////////////////////////////////////////


GeneratorModule GeneratorModule_load(const char *libname)
{
    GeneratorModule mod = {.valid = 1};
#ifdef USE_LOADLIBRARY
    mod.lib = LoadLibraryA(libname);
    if (mod.lib == 0 || mod.lib == INVALID_HANDLE_VALUE) {
        int errcode = (int) GetLastError();
        fprintf(stderr, "Cannot load the '%s' module; error code: %d\n",
            libname, errcode);
        mod.lib = 0;
        mod.valid = 0;
        return mod;
    }
#else
    mod.lib = dlopen(libname, RTLD_LAZY);
    if (mod.lib == 0) {
        fprintf(stderr, "dlopen() error: %s\n", dlerror());
        mod.lib = 0;
        mod.valid = 0;
        return mod;
    };
#endif

    GetGenInfoFunc gen_getinfo = (void *) DLSYM_WRAPPER(mod.lib, "gen_getinfo");
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        mod.valid = 0;
    }
    if (!gen_getinfo(&mod.gen)) {
        fprintf(stderr, "'gen_getinfo' function failed\n");
        mod.valid = 0;
    }

    return mod;
}


void GeneratorModule_unload(GeneratorModule *mod)
{
    mod->valid = 0;
    if (mod->lib != 0) {
        DLCLOSE_WRAPPER(mod->lib);
    }
}

///////////////////////////////
///// Sorting subroutines /////
///////////////////////////////


/**
 * @brief 16-bit counting sort for 64-bit arrays.
 */
static void countsort64(uint64_t *out, const uint64_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = (size_t *) calloc(65536, sizeof(size_t));
    for (size_t i = 0; i < len; i++) {
        unsigned int pos = ((x[i] >> shr) & 0xFFFF);
        offsets[pos]++;
    }
    for (size_t i = 1; i < 65536; i++) {
        offsets[i] += offsets[i - 1];
    }
    for (size_t i = len; i-- != 0; ) {
        unsigned int digit = ((x[i] >> shr) & 0xFFFF);
        size_t offset = --offsets[digit];
        out[offset] = x[i];
    }
    free(offsets);
}


/**
 * @brief 16-bit counting sort for 32-bit arrays.
 */
static void countsort32(uint32_t *out, const uint32_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = calloc(65536, sizeof(size_t));
    for (size_t i = 0; i < len; i++) {
        unsigned int pos = ((x[i] >> shr) & 0xFFFF);
        offsets[pos]++;
    }
    for (size_t i = 1; i < 65536; i++) {
        offsets[i] += offsets[i - 1];
    }
    for (size_t i = len; i-- != 0; ) {
        unsigned int digit = ((x[i] >> shr) & 0xFFFF);
        size_t offset = --offsets[digit];
        out[offset] = x[i];
    }
    free(offsets);
}


/**
 * @brief Radix sort for 64-bit unsigned integers.
 */
void radixsort64(uint64_t *x, size_t len)
{
    uint64_t *out = calloc(len, sizeof(uint64_t));
    countsort64(out, x,   len, 0);
    countsort64(x,   out, len, 16);
    countsort64(out, x,   len, 32);
    countsort64(x,   out, len, 48);
    free(out);
}

/**
 * @brief Radix sort for 32-bit unsigned integers.
 */
void radixsort32(uint32_t *x, size_t len)
{
    uint32_t *out = calloc(len, sizeof(uint32_t));
    countsort32(out, x,   len, 0);
    countsort32(x,   out, len, 16);
    free(out);
}


/**
 * @brief Converts the number of seconds to the hours/minutes/seconds
 * format. Useful for showing the elapsed time.
 */
TimeHMS nseconds_to_hms(unsigned long nseconds_total)
{
    TimeHMS hms;
    hms.h = (nseconds_total / 3600);
    hms.m = (nseconds_total / 60) % 60;
    hms.s = nseconds_total % 60;
    return hms;
}

/**
 * @brief Prints elapsed time in hh:mm:ss format to stdout.
 * @param nseconds_total  Number of seconds.
 */
void print_elapsed_time(unsigned long nseconds_total)
{    
    TimeHMS hms = nseconds_to_hms(nseconds_total);
    printf("%.2d:%.2d:%.2d", hms.h, hms.m, hms.s);
}


/////////////////////////////////////////////
///// TestsBattery class implementation /////
/////////////////////////////////////////////


size_t TestsBattery_ntests(const TestsBattery *obj)
{
    size_t ntests;
    for (ntests = 0; obj->tests[ntests].run != NULL; ntests++) { }
    return ntests;
}



typedef struct {
    pthread_t thrd_id;
    TestDescription *tests;
    size_t *tests_inds;
    size_t ntests;
    TestResults *results;
    const GeneratorInfo *gi;
    const CallerAPI *intf;
} BatteryThread;


static pthread_mutex_t battery_mutex = PTHREAD_MUTEX_INITIALIZER;

void *battery_thread(void *data)
{
    BatteryThread *th_data = data;
    pthread_mutex_lock(&battery_mutex);
    void *state = th_data->gi->create(th_data->intf);
    pthread_mutex_unlock(&battery_mutex);
    th_data->intf->printf("vvvvvvvvvv Thread %lld started vvvvvvvvvv\n", (int) th_data->thrd_id);
    GeneratorState obj = {.gi = th_data->gi, .state = state, .intf = th_data->intf};
    for (size_t i = 0; i < th_data->ntests; i++) {
        size_t ind = th_data->tests_inds[i];
        th_data->results[ind] = th_data->tests[i].run(&obj);
        th_data->results[ind].name = th_data->tests[i].name;
    }
    th_data->intf->printf("^^^^^^^^^^ Thread %lld finished ^^^^^^^^^^\n", (int) th_data->thrd_id);
    return NULL;
}



static void TestsBattery_run_threads(const TestsBattery *bat, size_t ntests,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int nthreads, TestResults *results)
{
    // Multithreaded version
    BatteryThread *th = calloc(nthreads, sizeof(BatteryThread));
    size_t *th_pos = calloc(nthreads, sizeof(size_t));
    // Preallocate arrays
    for (size_t i = 0; i < ntests; i++) {
        size_t ind = i % nthreads;
        th[ind].ntests++;
    }
    for (size_t i = 0; i < nthreads; i++) {
        th[i].tests = calloc(th->ntests, sizeof(TestDescription));
        th[i].tests_inds = calloc(th->ntests, sizeof(size_t));
        th[i].gi = gen;
        th[i].intf = intf;
        th[i].results = results;
    }
    // Dispatch tests
    for (size_t i = 0; i < ntests; i++) {
        size_t ind = i % nthreads;
        th[ind].tests[th_pos[ind]] = bat->tests[i];
        th[ind].tests_inds[th_pos[ind]++] = i;
    }
    // Run threads
    for (size_t i = 0; i < nthreads; i++) {        
        pthread_create(&th[i].thrd_id, NULL, battery_thread, &th[i]);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        pthread_join(th[i].thrd_id, NULL);
    }
    // Deallocate array
    for (size_t i = 0; i < nthreads; i++) {
        free(th[i].tests);
        free(th[i].tests_inds);
    }
    free(th);
    free(th_pos);
}


static void TestResults_print_report(const TestResults *results,
    size_t ntests, time_t nseconds_total)
{
    printf("  %3s %-20s %10s %10s %10s\n",
        "#", "Test name", "xemp", "p", "1 - p");
    for (size_t i = 0; i < 75; i++) {
        printf("-");
    }
    printf("\n");
    unsigned int npassed = 0, nwarnings = 0, nfailed = 0; 
    for (size_t i = 0; i < ntests; i++) {
        printf("  %3d %-20s %10.3g %10.3g %10.3g %10s\n",
            (int) i + 1, results[i].name, results[i].x, results[i].p,
            results[i].alpha,
            interpret_pvalue(results[i].p));
        switch (get_pvalue_category(results[i].p)) {
        case pvalue_passed:
            npassed++; break;
        case pvalue_warning:
            nwarnings++; break;
        case pvalue_failed:
            nfailed++; break;
        }
    }
    for (size_t i = 0; i < 75; i++) {
        printf("-");
    }
    printf("\n");
    printf("Passed:       %u\n", npassed);
    printf("Suspicious:   %u\n", nwarnings);
    printf("Failed:       %u\n", nfailed);
    printf("Elapsed time: "); print_elapsed_time(nseconds_total);
    printf("\n\n");
}

void TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    time_t tic, toc;
    void *state = gen->create(intf);
    TestResults *results = NULL;
    size_t ntests = TestsBattery_ntests(bat);
    printf("===== Starting '%s' battery =====\n", bat->name);
    GeneratorState obj = {.gi = gen, .state = state, .intf = intf};
    if (testid >= ntests) {
        fprintf(stderr, "Invalid test id %u", testid);
        return;
    }
    size_t nresults = ntests;
    tic = time(NULL);
    if (testid == TESTS_ALL) {
        results = calloc(ntests, sizeof(TestResults));
        nresults = ntests;
        if (nthreads == 1) {
            // One-threaded version
            for (size_t i = 0; i < ntests; i++) {
                results[i] = bat->tests[i].run(&obj);
                results[i].name = bat->tests[i].name;
            }
        } else {
            // Multithreaded version
            TestsBattery_run_threads(bat, ntests, gen, intf, nthreads, results);
        }
    } else {
        results = calloc(1, sizeof(TestResults));
        nresults = 1;
        *results = bat->tests[testid - 1].run(&obj);
        results->name = bat->tests[testid - 1].name;
    }
    toc = time(NULL);
    printf("\n");
    if (testid == TESTS_ALL) {
        printf("==================== '%s' battery report ====================\n", bat->name);
    } else {
        printf("==================== '%s' battery test #%d report ====================\n",
            bat->name, testid);
    }
    printf("Generator name:    %s\n", gen->name);
    printf("Output size, bits: %d\n\n", (int) gen->nbits);
    TestResults_print_report(results, nresults, toc - tic);
    free(results);
    free(state);
}

////////////////////////////////////////////////////////////////
///// Implementation of generator with reversed bits order /////
////////////////////////////////////////////////////////////////

static GeneratorInfo original_gen;

static uint64_t get_bits32_reversed(void *state)
{
    return reverse_bits32((uint32_t) original_gen.get_bits(state));
}

static uint64_t get_bits64_reversed(void *state)
{
    return reverse_bits64(original_gen.get_bits(state));
}

GeneratorInfo reversed_generator_set(const GeneratorInfo *gi)
{
    original_gen = *gi;
    GeneratorInfo reversed_gen = {.name = gi->name,
        .create = gi->create,
        .get_bits = (gi->nbits == 32) ? get_bits32_reversed : get_bits64_reversed,
        .nbits = gi->nbits, .self_test = gi->self_test};
    return reversed_gen;
}
