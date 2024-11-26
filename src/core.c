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
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#if defined(_MSC_VER) || defined(__WATCOMC__)
#include <io.h>
#endif
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

static Entropy entropy = {{0, 0, 0, 0}, 0, NULL, 0, 0};
static char cmd_param[128] = {0};
static int use_stderr_for_printf = 0;

static const char *get_cmd_param(void)
{
    return cmd_param;
}

void set_cmd_param(const char *param)
{
    strncpy(cmd_param, param, 128);
    cmd_param[127] = 0;
}

void set_use_stderr_for_printf(int val)
{
    use_stderr_for_printf = val;
}

///////////////////////////////
///// Single-threaded API /////
///////////////////////////////

static uint32_t get_seed32(void)
{
    return Entropy_seed64(&entropy, 0) >> 32;
}

static uint64_t get_seed64(void)
{
    return Entropy_seed64(&entropy, 0);
}

static int printf_ser(const char *format, ...)
{
    int ans;
    va_list args;
    va_start(args, format);
    if (use_stderr_for_printf) {
        ans = vfprintf(stderr, format, args);
    } else {
        ans = vprintf(format, args);
    }
    va_end(args);
    return ans;
}

CallerAPI CallerAPI_init(void)
{
    CallerAPI intf;
    if (entropy.state == 0) {
        Entropy_init(&entropy);
    }
    intf.get_seed32 = get_seed32;
    intf.get_seed64 = get_seed64;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_ser;
    intf.strcmp = strcmp;
    return intf;
}

void CallerAPI_free(void)
{
    Entropy_free(&entropy);
}

/////////////////////////////
///// Multithreaded API /////
/////////////////////////////

#if defined(USE_PTHREADS) || defined(USE_WINTHREADS)

#ifdef USE_PTHREADS
// Begin of pthreads-specific code
static pthread_mutex_t get_seed64_mt_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t printf_mt_mutex = PTHREAD_MUTEX_INITIALIZER;
static void init_mutexes()
{
}
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex);
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex);
static inline uint64_t get_current_thread_id()
{
    return (uint64_t) pthread_self();
}
static inline uint64_t get_thread_id(pthread_t handle)
{
    return (uint64_t) handle;
}
// End of pthreads-specific code
#else
// Begin of WinAPI-specific code
static HANDLE get_seed64_mt_mutex = NULL;
static HANDLE printf_mt_mutex = NULL;
static void init_mutexes()
{
    get_seed64_mt_mutex = CreateMutex(NULL, FALSE, "seed64_mutex");
    printf_mt_mutex = CreateMutex(NULL, FALSE, "printf_mutex");
}
#define MUTEX_LOCK(mutex) DWORD dwResult = WaitForSingleObject(mutex, INFINITE); \
    if (dwResult != WAIT_OBJECT_0) { \
        fprintf(stderr, "get_seed64_mt internal error"); \
        exit(1); \
    }
#define MUTEX_UNLOCK(mutex) ReleaseMutex(mutex);
static inline uint64_t get_current_thread_id()
{
    return (uint64_t) GetCurrentThreadId();
}
static inline uint64_t get_thread_id(DWORD id)
{
    return (uint64_t) id;
}
// End of WinAPI-specific code
#endif

static uint64_t get_seed64_mt(void)
{
    MUTEX_LOCK(get_seed64_mt_mutex);
    uint64_t seed = Entropy_seed64(&entropy, get_current_thread_id());
    MUTEX_UNLOCK(get_seed64_mt_mutex);
    return seed;
}

static uint32_t get_seed32_mt(void)
{
    return get_seed64_mt() >> 32;
}




static int printf_mt(const char *format, ...)
{
    int ans;
    MUTEX_LOCK(printf_mt_mutex);
    va_list args;
    va_start(args, format);
    if (use_stderr_for_printf) {
        fprintf(stderr, "=== THREAD #%2llu ===> ",
            (unsigned long long) get_current_thread_id());
        ans = vfprintf(stderr, format, args);
    } else {
        printf("=== THREAD #%2llu ===> ",
            (unsigned long long) get_current_thread_id());
        ans = vprintf(format, args);
    }
    va_end(args);
    MUTEX_UNLOCK(printf_mt_mutex);
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
        init_mutexes();
    }
    intf.get_seed32 = get_seed32_mt;
    intf.get_seed64 = get_seed64_mt;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_mt;
    intf.strcmp = strcmp;
    return intf;
}
#else
/**
 * @brief Return one-threaded version of functions for platforms
 * that don't support pthread.h
 */
CallerAPI CallerAPI_init_mthr(void)
{
    return CallerAPI_init();
}
#endif

/////////////////////////////////////////////
///// Some platform-dependent functions /////
/////////////////////////////////////////////

int get_cpu_numcores(void)
{
#ifdef USE_LOADLIBRARY
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

////////////////////////////////////////////
///// TestResults class implementation /////
////////////////////////////////////////////

TestResults TestResults_create(const char *name)
{
    TestResults ans;
    ans.name = name;
    ans.id = 0;
    ans.p = NAN;
    ans.alpha = NAN;
    ans.x = NAN;
    ans.thread_id = 0;
    return ans;
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

//////////////////////////////////////////////////
///// Subroutines for working with C modules /////
//////////////////////////////////////////////////


/**
 * @brief Loads shared library (`.so` or `.dll`) with module that
 * implements pseudorandom number generator.
 */
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

    GetGenInfoFunc gen_getinfo = (GetGenInfoFunc)( (void *) DLSYM_WRAPPER(mod.lib, "gen_getinfo") );
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        mod.valid = 0;
    } else if (!gen_getinfo(&mod.gen)) {
        fprintf(stderr, "'gen_getinfo' function failed\n");
        mod.valid = 0;
    }
    return mod;
}

/**
 * @brief Unloads shared library (`.so` or `.dll`) with module that
 * implements pseudorandom number generator.
 */
void GeneratorModule_unload(GeneratorModule *mod)
{
    mod->valid = 0;
    if (mod->lib != 0) {
        DLCLOSE_WRAPPER(mod->lib);
    }
}

///////////////////////////////////////
///// Hamming weights subroutines /////
///////////////////////////////////////


///////////////////////////////
///// Sorting subroutines /////
///////////////////////////////


/**
 * @brief 16-bit counting sort for 64-bit arrays.
 */
static void countsort64(uint64_t *out, const uint64_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = (size_t *) calloc(65536, sizeof(size_t));
    if (offsets == NULL) {
        fprintf(stderr, "***** countsort64: not enough memory *****\n");
        exit(1);
    }
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
    if (offsets == NULL) {
        fprintf(stderr, "***** countsort32: not enough memory *****\n");
        exit(1);
    }
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


#ifndef NOTHREADS

typedef struct {
#ifdef USE_PTHREADS
    pthread_t thrd_id;
#elif defined(USE_WINTHREADS)
    DWORD thrd_id;
    HANDLE thrd_handle;
#endif
    TestDescription *tests;
    size_t *tests_inds;
    size_t ntests;
    TestResults *results;
    const GeneratorInfo *gi;
    const CallerAPI *intf;
} BatteryThread;

#ifdef USE_WINTHREADS
static DWORD WINAPI battery_thread(void *data)
#else
static void *battery_thread(void *data)
#endif
{
    BatteryThread *th_data = data;
    th_data->intf->printf("vvvvvvvvvv Thread %lld started vvvvvvvvvv\n",
        get_thread_id(th_data->thrd_id));
    GeneratorState obj = GeneratorState_create(th_data->gi, th_data->intf);
    for (size_t i = 0; i < th_data->ntests; i++) {
        size_t ind = th_data->tests_inds[i];
        th_data->intf->printf("vvvvv Thread %lld: test %s started vvvvv\n",
            get_thread_id(th_data->thrd_id), th_data->tests[i].name);
        th_data->results[ind] = th_data->tests[i].run(&obj);
        th_data->intf->printf("^^^^^ Thread %lld: test %s finished ^^^^^\n",
            get_thread_id(th_data->thrd_id), th_data->tests[i].name);
        th_data->results[ind].name = th_data->tests[i].name;
        th_data->results[ind].id = ind + 1;
        th_data->results[ind].thread_id = get_current_thread_id();
    }
    th_data->intf->printf("^^^^^^^^^^ Thread %lld finished ^^^^^^^^^^\n",
        get_thread_id(th_data->thrd_id));
    GeneratorState_free(&obj, th_data->intf);
    return 0;
}

typedef struct {
    size_t ind; ///< Text index in the array
    unsigned int nseconds; ///< Estimated time, seconds
} TestTiming;


static int cmp_TestTiming(const void *aptr, const void *bptr)
{
    const TestTiming *o1 = aptr, *o2 = bptr;
    if (o1->nseconds < o2->nseconds) { return 1; }
    else if (o1->nseconds == o2->nseconds) { return 0; }
    else { return -1; }
}


/**
 * @brief Sorts tests by the estimated execution time in the descending order.
 * It is used to distribute them between threads.
 */
static TestTiming *sort_tests_by_time(const TestDescription *descr, size_t ntests)
{
    TestTiming *out = calloc(ntests, sizeof(TestTiming));
    if (out == NULL) {
        fprintf(stderr, "sort_tests_by_time: out of memory");
        exit(1);
    }
    for (size_t i = 0; i < ntests; i++) {
        out[i].ind = i;
        out[i].nseconds = descr[i].nseconds;
    }
    qsort(out, ntests, sizeof(TestTiming), cmp_TestTiming);
    return out;
}

#endif
// NOTHREADS



static inline size_t test_pos_to_thread_ind(size_t test_pos, size_t nthreads)
{
    size_t thr_ind;
    if ((test_pos / nthreads) % 2 == 0) {
        thr_ind = test_pos % nthreads;
    } else {
        thr_ind = (nthreads - 1) - test_pos % nthreads;
    }
    return thr_ind;
}

/**
 * @brief Run the test battery in the multithreaded mode.
 */
static void TestsBattery_run_threads(const TestsBattery *bat, size_t ntests,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int nthreads, TestResults *results)
{
#ifndef NOTHREADS
    // Multithreaded version
    BatteryThread *th = calloc(nthreads, sizeof(BatteryThread));
    size_t *th_pos = calloc(nthreads, sizeof(size_t));
    // Preallocate arrays
    for (size_t i = 0; i < ntests; i++) {
        size_t ind = test_pos_to_thread_ind(i, nthreads);
        th[ind].ntests++;
    }
    for (size_t i = 0; i < nthreads; i++) {
        th[i].tests = calloc(th[i].ntests, sizeof(TestDescription));
        th[i].tests_inds = calloc(th[i].ntests, sizeof(size_t));
        th[i].gi = gen;
        th[i].intf = intf;
        th[i].results = results;
    }
    // Dispatch tests (and try to balance them between threads
    // according to estimations of elapsed time)
    // 0,1,...,(nthreads-1),(nthreads-1),...,2,1,0,0,1,2,....
    TestTiming *tt = sort_tests_by_time(bat->tests, ntests);
    for (size_t i = 0; i < ntests; i++) {
        size_t thr_ind = test_pos_to_thread_ind(i, nthreads);
        size_t test_ind = tt[i].ind;
        th[thr_ind].tests[th_pos[thr_ind]] = bat->tests[test_ind];
        th[thr_ind].tests_inds[th_pos[thr_ind]] = test_ind;
        th_pos[thr_ind]++;
    }
    free(tt);
    // Show balance
    for (size_t i = 0; i < nthreads; i++) {
        printf("Thread %d: ", (int) (i + 1));
        for (size_t j = 0; j < th[i].ntests; j++) {
            size_t test_ind = th[i].tests_inds[j];
            printf("%d(%s) ", (int) test_ind, bat->tests[test_ind].name);
        }
        printf("\n");
    }
#ifdef USE_PTHREADS
    // Run threads
    for (size_t i = 0; i < nthreads; i++) {
        pthread_create(&th[i].thrd_id, NULL, battery_thread, &th[i]);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        pthread_join(th[i].thrd_id, NULL);
    }
#elif defined(USE_WINTHREADS)
    // Run threads
    for (size_t i = 0; i < nthreads; i++) {
        th[i].thrd_handle = CreateThread(NULL, 0, battery_thread, &th[i], 0, &th[i].thrd_id);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        WaitForSingleObject(th[i].thrd_handle, INFINITE);
    }
#endif
    // Deallocate array
    for (size_t i = 0; i < nthreads; i++) {
        free(th[i].tests);
        free(th[i].tests_inds);
    }
    free(th);
    free(th_pos);
#else
    (void) ntests;
    (void) nthreads;
    (void) results;
    printf("WARNING: multithreading is not supported on this platform\n");
    printf("Rerunning in one-threaded mode\n");
    TestsBattery_run(bat, gen, intf, TESTS_ALL, 1);
#endif
}


static void snprintf_pvalue(char *buf, size_t len, double p, double alpha)
{
    if (p != p || alpha != alpha) {
        snprintf(buf, len, "NAN");
    } else if (p < 0.0 || p > 1.0) {
        snprintf(buf, len, "???");
    } else if (p < DBL_MIN) {
        snprintf(buf, len, "0");
    } else if (1.0e-3 <= p && p <= 0.999) {
        snprintf(buf, len, "%.3f", p);
    } else if (p < 1.0e-3) {
        snprintf(buf, len, "%.2e", p);
    } else if (p > 0.999 && alpha > DBL_MIN) {
        snprintf(buf, len, "1 - %.2e", alpha);
    } else {
        snprintf(buf, len, "1");
    }
}

static void print_bar()
{
    for (int i = 0; i < 79; i++) {
        printf("-");
    }
    printf("\n");
}


static void TestResults_print_report(const TestResults *results,
    size_t ntests, time_t nseconds_total)
{
    unsigned int npassed = 0, nwarnings = 0, nfailed = 0;
    printf("  %3s %-20s %12s %14s %-15s %4s\n",
        "#", "Test name", "xemp", "p", "Interpretation", "Thr#");
    print_bar();
    for (size_t i = 0; i < ntests; i++) {
        char pvalue_txt[32];
        snprintf_pvalue(pvalue_txt, 32, results[i].p, results[i].alpha);
        printf("  %3u %-20s %12g %14s %-15s %4llu\n",
            results[i].id, results[i].name, results[i].x, pvalue_txt,
            interpret_pvalue(results[i].p),
            (unsigned long long) results[i].thread_id);
        switch (get_pvalue_category(results[i].p)) {
        case pvalue_passed:
            npassed++; break;
        case pvalue_warning:
            nwarnings++; break;
        case pvalue_failed:
            nfailed++; break;
        }
    }
    print_bar();
    printf("Passed:       %u\n", npassed);
    printf("Suspicious:   %u\n", nwarnings);
    printf("Failed:       %u\n", nfailed);
    printf("Elapsed time: "); print_elapsed_time(nseconds_total);
    printf("\n\n");
}

/**
 * @brief Prints a summary about the test battery to stdout.
 */
void TestsBattery_print_info(const TestsBattery *obj)
{
    size_t ntests = TestsBattery_ntests(obj);
    unsigned long nseconds_total = 0;
    printf("===== Battery '%s' summary =====\n", obj->name);
    printf("  %3s %-20s %-20s\n",
        "#", "Test name", "Estimated time, sec");
    print_bar();
    for (size_t i = 0; i < ntests; i++) {
        printf("  %3d %-20s %6u\n",
            (int) i + 1, obj->tests[i].name, obj->tests[i].nseconds);
        nseconds_total += obj->tests[i].nseconds;
    }
    print_bar();
    printf("Estimated elapsed time (one-threaded): ");
    print_elapsed_time(nseconds_total);
    printf("\n\n");
}

/**
 * @brief Runs the given battery of the statistical test for the given
 * pseudorandom number generator.
 */
void TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    time_t tic, toc;
    size_t ntests = TestsBattery_ntests(bat);
    size_t nresults = ntests;
    TestResults *results = NULL;
#ifdef NOTHREADS
    nthreads = 1;
#endif
    printf("===== Starting '%s' battery =====\n", bat->name);
    if (testid > ntests) { // testid is 1-based index
        fprintf(stderr, "Invalid test id %u", testid);
        return;
    }
    // Allocate memory for tests
    if (testid == TESTS_ALL) {
        results = calloc(ntests, sizeof(TestResults));
        nresults = ntests;
    } else {
        results = calloc(1, sizeof(TestResults));
        nresults = 1;
    }
    if (results == NULL) {
        fprintf(stderr, "***** TestsBattery_run: not enough memory *****\n");
        exit(1);
    }
    // Run the tests
    tic = time(NULL);
    if (nthreads == 1 || testid != TESTS_ALL) {
        // One-threaded version
        GeneratorState obj = GeneratorState_create(gen, intf);
        if (testid == TESTS_ALL) {
            for (size_t i = 0; i < ntests; i++) {
                results[i] = bat->tests[i].run(&obj);
                results[i].name = bat->tests[i].name;
                results[i].id = i + 1;
                results[i].thread_id = 0;
            }
        } else {
            *results = bat->tests[testid - 1].run(&obj);
            results->name = bat->tests[testid - 1].name;
            results->id = testid;
        }
        GeneratorState_free(&obj, intf);
    } else {
       // Multithreaded version
       TestsBattery_run_threads(bat, ntests, gen, intf, nthreads, results);
    }
    toc = time(NULL);
    printf("\n");
    printf("==================== Seeds logger report ====================\n");
    Entropy_print_seeds_log(&entropy, stdout);
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
}

//////////////////////////////////////////////////////////////////
///// Some generic functions for making enveloped generators /////
//////////////////////////////////////////////////////////////////

/**
 * @brief Buffer for decomposition of each 64-bit value into a pair
 * of 32-bit values. Needed for TestU01: its batteries should have
 * an access to all bits. Used by MAKE_UINT64_INTERLEAVED_PRNG macro.
 */
typedef struct {
    union {
        uint64_t u64; ///< To be splitted to two 32-bit values.
        uint32_t u32[2]; ///< 32-bit values.
    } val; ///< Internal buffer for 32-bit outputs.    
    int pos; ///< Output position for 32-bit output.
} Interleaved32Buffer;


typedef struct {
    const GeneratorInfo *parent_gi;
    void *parent_state;
    Interleaved32Buffer i32buf;
} EnvelopedGeneratorState;


void *create_enveloped(const GeneratorInfo *gi, const CallerAPI *intf)
{
    EnvelopedGeneratorState *obj = intf->malloc(sizeof(EnvelopedGeneratorState));
    obj->parent_gi = gi->parent;
    obj->parent_state = gi->parent->create(gi->parent, intf);
    obj->i32buf.pos = 2; // Invalidate buffer for interleaved output
    return obj;
}

void free_enveloped(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    EnvelopedGeneratorState *obj = state;
    gi->parent->free(obj->parent_state, gi->parent, intf);
    intf->free(state);
}

////////////////////////////////////////////////////////////////
///// Implementation of generator with reversed bits order /////
////////////////////////////////////////////////////////////////


static uint64_t get_bits32_reversed(void *state)
{
    EnvelopedGeneratorState *obj = state;
    return reverse_bits32((uint32_t) obj->parent_gi->get_bits(obj->parent_state));
}

static uint64_t get_bits64_reversed(void *state)
{
    EnvelopedGeneratorState *obj = state;
    return reverse_bits64((uint64_t) obj->parent_gi->get_bits(obj->parent_state));
}

/**
 * @brief Defines the PRNG with reversed bits order.
 */
GeneratorInfo define_reversed_generator(const GeneratorInfo *gi)
{
    GeneratorInfo gi_env = *gi;
    gi_env.name = "ReversedBits";
    gi_env.parent = gi;
    gi_env.create = create_enveloped;
    gi_env.free = free_enveloped;
    if (gi->nbits == 32) {
        gi_env.get_bits = get_bits32_reversed;
    } else {
        gi_env.get_bits = get_bits64_reversed;
    }
    gi_env.get_sum = NULL;
    return gi_env;
}


///////////////////////////////////////////////////////////////////
///// Implementation of generator with interleaved bits order /////
///////////////////////////////////////////////////////////////////

static uint64_t get_bits32_interleaved(void *state)
{
    EnvelopedGeneratorState *obj = state;
    if (obj->i32buf.pos == 2) {
        uint64_t u = obj->parent_gi->get_bits(obj->parent_state);
        obj->i32buf.val.u64 = u;
        obj->i32buf.pos = 0;
    }
    return obj->i32buf.val.u32[obj->i32buf.pos++];
}

GeneratorInfo define_interleaved_generator(const GeneratorInfo *gi)
{
    GeneratorInfo gi_env = *gi;
    gi_env.name = "Interleaved";
    gi_env.parent = gi;
    gi_env.nbits = 32;
    gi_env.create = create_enveloped;
    gi_env.free = free_enveloped;
    gi_env.get_bits = get_bits32_interleaved;
    gi_env.get_sum = NULL;
    return gi_env;
}


///////////////////////////////////////////////
///// Implementation of file input/output /////
///////////////////////////////////////////////


/**
 * @brief Switches stdout to binary mode in MS Windows (needed for
 * correct output of binary data)
 */
void set_bin_stdout()
{
#ifdef USE_LOADLIBRARY
    (void) _setmode( _fileno(stdout), _O_BINARY);
#endif
}


/**
 * @brief Switches stdin to binary mode in MS Windows (needed for
 * correct input of binary data)
 */
void set_bin_stdin()
{
#ifdef USE_LOADLIBRARY
    (void) _setmode( _fileno(stdin), _O_BINARY);
#endif
}


/**
 * @brief Dump an output PRNG to the stdout in the format suitable
 * for PractRand.
 */
void GeneratorInfo_bits_to_file(GeneratorInfo *gen, const CallerAPI *intf)
{
    set_bin_stdout();
    void *state = gen->create(gen, intf);
    if (gen->nbits == 32) {
        uint32_t buf[256];
        while (1) {
            for (size_t i = 0; i < 256; i++) {
                buf[i] = gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint32_t), 256, stdout);
        }
    } else {
        uint64_t buf[256];
        while (1) {
            for (size_t i = 0; i < 256; i++) {
                buf[i] = gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint64_t), 256, stdout);
        }
    }
}
