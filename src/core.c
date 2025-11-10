/**
 * @file core.c
 * @brief Subroutines and special functions required for implementation
 * of statistical tests.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/base64.h"
#include "smokerand/core.h"
#include "smokerand/entropy.h"
#include "smokerand/specfuncs.h"
#include "smokerand/threads_intf.h"
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

#define THREAD_ORD_OFFSET 1

static Entropy entropy = ENTROPY_INITIALIZER;
static char cmd_param[128] = {0};
static int use_stderr_for_printf = 0;
static int use_mutexes = 0;
static unsigned int seed64_mt_current_thread_ord = 0;

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

void set_entropy_textseed(const char *seed)
{
    Entropy_init_from_textseed(&entropy, seed);
}

int set_entropy_base64_seed(const char *seed)
{
    return Entropy_init_from_base64_seed(&entropy, seed);
}


///////////////////////////////
///// Single-threaded API /////
///////////////////////////////

static uint32_t get_seed32(void)
{
    return (uint32_t) (Entropy_seed64(&entropy, 0) >> 32);
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
    use_mutexes = 0;
    intf.get_seed32 = get_seed32;
    intf.get_seed64 = get_seed64;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_ser;
    intf.snprintf = snprintf;
    intf.strcmp = strcmp;
    intf.get_ram_info = get_ram_info;
    return intf;
}

/////////////////////////////
///// Multithreaded API /////
/////////////////////////////

DECLARE_MUTEX(get_seed64_mt_mutex)
DECLARE_MUTEX(printf_mt_mutex)

static void init_mutexes()
{
    INIT_MUTEX(get_seed64_mt_mutex)
    INIT_MUTEX(printf_mt_mutex)
}

static void destroy_mutexes()
{
    MUTEX_DESTROY(get_seed64_mt_mutex)
    MUTEX_DESTROY(printf_mt_mutex)    
}

static uint64_t get_seed64_mt(void)
{
    MUTEX_LOCK(get_seed64_mt_mutex);
    unsigned int ord = seed64_mt_current_thread_ord;
    if (ord < THREAD_ORD_OFFSET) {
        ThreadObj thr = ThreadObj_current();
        ord = thr.ord;
    }
    const uint64_t seed = Entropy_seed64(&entropy, ord);
    MUTEX_UNLOCK(get_seed64_mt_mutex);
    return seed;
}

static uint32_t get_seed32_mt(void)
{
    return (uint32_t) (get_seed64_mt() >> 32);
}

static int printf_mt(const char *format, ...)
{
    int ans;
    ThreadObj thr = ThreadObj_current();
    MUTEX_LOCK(printf_mt_mutex);
    va_list args;
    va_start(args, format);    
    if (use_stderr_for_printf) {
        fprintf(stderr, "=== THREAD #%2u ===> ", thr.ord);
        ans = vfprintf(stderr, format, args);
    } else {
        printf("=== THREAD #%2u ===> ", thr.ord);
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
    init_mutexes();
    use_mutexes = 1;
    intf.get_seed32 = get_seed32_mt;
    intf.get_seed64 = get_seed64_mt;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_mt;
    intf.snprintf = snprintf;
    intf.strcmp = strcmp;
    intf.get_ram_info = get_ram_info;
    return intf;
}

///////////////////////////////////////////////////////////////////////////
///// Universal API (for both single- and multithreaded environments) /////
///////////////////////////////////////////////////////////////////////////

void CallerAPI_free(void)
{
    Entropy_free(&entropy);
    if (use_mutexes) {
        destroy_mutexes();
    }
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
    ans.penalty = 0;
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
        return PVALUE_FAILED;
    } else if (pvalue < pvalue_warn_val || pvalue > 1.0 - pvalue_warn_val) {
        return PVALUE_WARNING;
    } else {
        return PVALUE_PASSED;
    }
}

const char *interpret_pvalue(double pvalue)
{
    switch (get_pvalue_category(pvalue)) {
    case PVALUE_FAILED:
        return "FAIL";
    case PVALUE_WARNING:
        return "SUSPICIOUS";
    case PVALUE_PASSED:
        return "Ok";
    default:
        return "???";
    }
}


///////////////////////////////////////////////
///// GeneratorState class implementation /////
///////////////////////////////////////////////

GeneratorState GeneratorState_create(const GeneratorInfo *gi,
    const CallerAPI *intf)
{
    GeneratorState obj;
    obj.gi = gi;
    obj.state = gi->create(gi, intf);
    obj.intf = intf;
    if (obj.state == NULL) {
        fprintf(stderr,
            "Cannot create an example of generator '%s' with parameter '%s'\n",
            gi->name, intf->get_param());
        exit(EXIT_FAILURE);
    }
    return obj;
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

/**
 * @brief Destructor for the generator state: deallocates all internal
 * buffers but not the GeneratorState itself.
 */
void GeneratorState_destruct(GeneratorState *obj)
{
    obj->gi->free(obj->state, obj->gi, obj->intf);
}

/**
 * @brief Checks if the generator output size is consistent
 * with the number of bits.
 */
int GeneratorState_check_size(const GeneratorState *obj)
{
    if (obj->gi->nbits == 64) {
        return 1;
    } else {
        for (size_t i = 0; i < 1000; i++) {
            uint64_t x = obj->gi->get_bits(obj->state);
            if (x >> obj->gi->nbits != 0) {
                return 0;
            }
        }
    }
    return 1;
}


//////////////////////////////////////////////////
///// Subroutines for working with C modules /////
//////////////////////////////////////////////////


/**
 * @brief Loads shared library (`.so` or `.dll`) with module that
 * implements pseudorandom number generator.
 */
GeneratorModule GeneratorModule_load(const char *libname, const CallerAPI *intf)
{
    GeneratorModule mod = {.valid = 1};
    mod.lib = dlopen_wrap(libname);
    if (mod.lib == NULL) {
        mod.valid = 0;
        return mod;
    }
    void *fptr = dlsym_wrap(mod.lib, "gen_getinfo");
    GetGenInfoFunc gen_getinfo;
    memcpy(&gen_getinfo, &fptr, sizeof(gen_getinfo));
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        mod.valid = 0;
    } else {
        int is_ok = gen_getinfo(&mod.gen, intf);
        if (!is_ok) {
            fprintf(stderr, "'gen_getinfo' function failed (%d returned)\n", is_ok);
            mod.valid = 0;
        }
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
    if (mod->lib != NULL) {
        dlclose_wrap(mod->lib);
    }
}

///////////////////////////////
///// Sorting subroutines /////
///////////////////////////////


static void insertsort(uint64_t *x, ptrdiff_t begin, ptrdiff_t end)
{
    ptrdiff_t i;
    for (i = begin + 1; i <= end; i++) {
        uint64_t xi = x[i];
        ptrdiff_t j = i;
        while (j > begin && x[j - 1] > xi) {
            x[j] = x[j - 1];
            j--;
        }
        x[j] = xi;
    }
}

static void quicksort_range(uint64_t *v, ptrdiff_t begin, ptrdiff_t end)
{
    ptrdiff_t i = begin, j = end, med = (begin + end) / 2;
    uint64_t pivot = v[med];
    while (i <= j) {
        if (v[i] < pivot) {
            i++;
        } else if (v[j] > pivot) {
            j--;
        } else {
            uint64_t tmp = v[i];
            v[i++] = v[j];
            v[j--] = tmp;
        }
    }
    if (begin < j) {
        if (j - begin > 12) {
            quicksort_range(v, begin, j);
        } else {
            insertsort(v, begin, j);
        }
    }
    if (end > i) {
        if (end - i > 12) {
            quicksort_range(v, i, end);
        } else {
            insertsort(v, i, end);
        }
    }
}


void quicksort64(uint64_t *x, size_t len)
{
    quicksort_range(x, 0, (ptrdiff_t) (len - 1));
}


/**
 * @brief 16-bit counting sort for 64-bit arrays.
 */
static void countsort64(uint64_t *out, const uint64_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = (size_t *) calloc(65536, sizeof(size_t));
    if (offsets == NULL) {
        fprintf(stderr, "***** countsort64: not enough memory *****\n");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
    if (out == NULL) {
        // Not enough memory for a buffer: use another algorithm.
        // May be useful for platforms with low amounts of RAM and no virtual
        // memory such as 32-bit DOS extenders.
        quicksort64(x, len);
        return;
    }
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
    if (out == NULL) {
        fprintf(stderr, "***** radixsort32: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    countsort32(out, x,   len, 0);
    countsort32(x,   out, len, 16);
    free(out);
}


/**
 * @brief Fast sort for 64-bit integers that selects between radix sort
 * and quick sort. Selection depends on the available RAM estimation
 * and will use quicksort if we don't have enough memory for buffers.
 */
void fastsort64(const RamInfo *info, uint64_t *x, size_t len)
{
    const long long bufsize = (long long) (len * sizeof(uint64_t)) + (1ll << 20);
    const long long ramsize = info->phys_avail_nbytes;
    if (ramsize != RAM_SIZE_UNKNOWN && bufsize > ramsize) {
        radixsort64(x, len);
    } else {
        quicksort64(x, len);
    }
}


/**
 * @brief Converts the number of seconds to the hours/minutes/seconds
 * format. Useful for showing the elapsed time.
 */
TimeHMS nseconds_to_hms(unsigned long long nseconds_total)
{
    TimeHMS hms;
    hms.h = (unsigned int) (nseconds_total / 3600);
    hms.m = (unsigned short) ((nseconds_total / 60) % 60);
    hms.s = (unsigned short) (nseconds_total % 60);
    return hms;
}

/**
 * @brief Prints elapsed time in hh:mm:ss format to stdout.
 * @param nseconds_total  Number of seconds.
 */
void print_elapsed_time(unsigned long long nseconds_total)
{
    TimeHMS hms = nseconds_to_hms(nseconds_total);
    printf("%.2d:%.2d:%.2d", hms.h, hms.m, hms.s);
}


typedef struct {
    size_t ind; ///< Test index (ID) inside battery and in the output buffer
    size_t ord; ///< Test ordinal (for output information)
} TestIndex;

////////////////////////////////////////////
///// ThreadQueue class implementation /////
////////////////////////////////////////////

typedef struct {
    GeneratorState gen; ///< Per-thread PRNG
    TestIndex *tests_inds; ///< Test indexes and ordinals
    size_t ntests;
    size_t front;
    unsigned int thread_ord; ///< Thread ordinal
} ThreadQueue;


void ThreadQueue_init(ThreadQueue *obj, const GeneratorInfo *gi,
    const CallerAPI *intf, unsigned int thread_ord)
{
    obj->ntests = 0;
    obj->front = 0;
    obj->thread_ord = thread_ord;
    obj->tests_inds = calloc(1, sizeof(TestIndex));
    obj->gen = GeneratorState_create(gi, intf);
}


void ThreadQueue_push_back(ThreadQueue *obj, TestIndex index)
{
    obj->ntests++;
    obj->tests_inds = realloc(obj->tests_inds, obj->ntests * sizeof(TestIndex));
    obj->tests_inds[obj->ntests - 1] = index;
}


TestIndex ThreadQueue_pop_front(ThreadQueue *obj)
{
    static const TestIndex none = {.ind = SIZE_MAX, .ord = SIZE_MAX};
    return (obj->front == obj->ntests) ? none : obj->tests_inds[obj->front++];
}


void ThreadQueue_destruct(ThreadQueue *obj)
{
    GeneratorState_destruct(&obj->gen);
    free(obj->tests_inds);
}

////////////////////////////////////////////////
///// TestsDispatcher class implementation /////
////////////////////////////////////////////////

/**
 * @brief State of multi-threaded tests dispatcher. It uses PRNG from entropy
 * module but its result is fully determined by the PRNG input. This determinism
 * allows to obtain completely reproducible results (e.g. p-values in the test
 * reports) from the same seed.
 */
typedef struct {
    size_t *res_thrd_ord;
    const TestsBattery *bat;
    size_t ntests;
    TestResults *results;
    const GeneratorInfo *gi;
    const CallerAPI *intf;
    ThreadQueue *queues;
    unsigned int nthreads;
} TestsDispatcher;


void TestsDispatcher_init(TestsDispatcher *obj, const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int nthreads, TestResults *results,
    int shuffle)
{
    size_t ntests = TestsBattery_ntests(bat);
    obj->res_thrd_ord = calloc(ntests, sizeof(size_t));
    obj->bat = bat;
    obj->ntests = ntests;
    obj->results = results;
    obj->gi = gen;
    obj->intf = intf;
    obj->nthreads = nthreads;
    obj->queues = calloc(nthreads, sizeof(ThreadQueue));

    // They should be initialized BEFORE shuffling: shuffling
    // uses get_seed64 from entropy generator.
    for (unsigned int i = 0; i < nthreads; i++) {
        const unsigned int thread_ord = i + THREAD_ORD_OFFSET;
        seed64_mt_current_thread_ord = thread_ord;
        ThreadQueue_init(&obj->queues[i], gen, intf, thread_ord);
    }
    seed64_mt_current_thread_ord = 0;
    // Prepare an unshuffled array of indexes
    TestIndex *inds = calloc(ntests, sizeof(TestIndex));
    for (size_t i = 0; i < ntests; i++) {
        inds[i].ind = i;
        inds[i].ord = i + THREAD_ORD_OFFSET;
    }
    // An optional shuffling (shuffling indexes but not ordinals)
    if (shuffle) {
        uint64_t state = intf->get_seed64();
        for (size_t i = 0; i < ntests; i++) {
            state = pcg_bits64(&state);
            size_t j = (size_t) (state % ntests);
            size_t tmp = inds[i].ind;
            inds[i].ind = inds[j].ind;
            inds[j].ind = tmp;
        }
    }
    // Assign tests to threads
    unsigned int thr_id = 0;
    for (size_t i = 0; i < ntests; i++) {
        thr_id = (thr_id + 1) % nthreads;
        ThreadQueue_push_back(&obj->queues[thr_id], inds[i]);
    }
    // Free internal buffers
    free(inds);
}


ThreadQueue *
TestsDispatcher_get_queue_by_ord(const TestsDispatcher *obj, unsigned int ord)
{
    for (unsigned int i = 0; i < obj->nthreads; i++) {
        if (obj->queues[i].thread_ord == ord) {
            return &obj->queues[i];
        }
    }
    return NULL;
}



void TestsDispatcher_destruct(TestsDispatcher *obj)
{
    //free(obj->inds);
    free(obj->res_thrd_ord);
    for (unsigned int i = 0; i < obj->nthreads; i++) {
        ThreadQueue_destruct(&obj->queues[i]);
    }
    free(obj->queues);
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


static ThreadRetVal THREADFUNC_SPEC battery_thread(void *data)
{
    TestsDispatcher *th_data = data;
    const TestsBattery *bat = th_data->bat;
    ThreadObj thrd = ThreadObj_current();
    th_data->intf->printf("vvvvvvvvvv Thread %u (ID %llu) started vvvvvvvvvv\n",
        thrd.ord, (unsigned long long) thrd.id);

    ThreadQueue *queue = TestsDispatcher_get_queue_by_ord(th_data, thrd.ord);
    if (queue == NULL) {
        fprintf(stderr, "***** Thread %u: cannot find queue *****\n", thrd.ord);
        exit(EXIT_FAILURE);
    }
    for (TestIndex ti = ThreadQueue_pop_front(queue);
        ti.ind < th_data->ntests;
        ti = ThreadQueue_pop_front(queue))
    {
        th_data->intf->printf(
            "vvvvv Thread %u: test #%lld: %s (%lld of %lld) started vvvvv\n",
            thrd.ord,
            (long long) ti.ind + 1, bat->tests[ti.ind].name,
            (long long) ti.ord + 1, (long long) th_data->ntests);
        th_data->results[ti.ind] = TestDescription_run(&bat->tests[ti.ind], &queue->gen);
        th_data->res_thrd_ord[ti.ind] = thrd.ord;
        th_data->intf->printf(
            "^^^^^ Thread %u: test #%lld: %s (%lld of %lld) finished ^^^^^\n",
            thrd.ord,
            (long long) ti.ind + 1, bat->tests[ti.ind].name,
            (long long) ti.ord + 1, (long long) th_data->ntests);
        th_data->results[ti.ind].name = bat->tests[ti.ind].name;
        th_data->results[ti.ind].id = (unsigned int) (ti.ind + 1);
        th_data->results[ti.ind].thread_id = thrd.ord;
    }
    th_data->intf->printf("^^^^^^^^^^ Thread %u (ID %llu) finished ^^^^^^^^^^\n",
        thrd.ord, (unsigned long long) thrd.id);
    return 0;
}


/**
 * @brief Run the test battery in the multithreaded mode.
 */
static void TestsBattery_run_threads(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int nthreads, TestResults *results, ReportType rtype)
{
    TestsDispatcher tdisp;
    TestsDispatcher_init(&tdisp, bat, gen, intf, nthreads, results, 1);
    // Run threads
    init_thread_dispatcher();
    ThreadObj *thrd = calloc(sizeof(ThreadObj), nthreads);
    for (unsigned int i = 0; i < nthreads; i++) {
        const unsigned int ord = tdisp.queues[i].thread_ord;
        thrd[i] = ThreadObj_create(battery_thread, &tdisp, ord);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        ThreadObj_wait(&thrd[i]);
    }
    // Deallocate array
    TestsDispatcher_destruct(&tdisp);
    free(thrd);
    (void) rtype;
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

static const char *interpret_grade(double penalty)
{
    if (penalty >= 3.999) {
        return "good";
    } else if (penalty >= 3.0) {
        return "some issues";
    } else if (penalty >= 2.0) {
        return "flawed";
    } else if (penalty >= 1.0) {
        return "bad";
    } else {
        return "very bad";
    }
}

static void print_bar(void)
{
    for (int i = 0; i < 79; i++) {
        printf("-");
    }
    printf("\n");
}


typedef struct {
    unsigned int npassed;
    unsigned int nwarnings;
    unsigned int nfailed;
    double grade;
    const char *grade_text;
} TestResultsSummary;


static void
TestResultsSummary_fill(TestResultsSummary *obj, const TestResults *results, size_t ntests)
{
    obj->npassed = 0;
    obj->nwarnings = 0;
    obj->nfailed = 0;
    obj->grade = 4.0;
    for (size_t i = 0; i < ntests; i++) {
        PValueCategory pvalue_cat = get_pvalue_category(results[i].p);
        switch (pvalue_cat) {
        case PVALUE_PASSED:
            obj->npassed++; break;
        case PVALUE_WARNING:
            obj->nwarnings++; break;
        case PVALUE_FAILED:
            obj->nfailed++;
            obj->grade -= results[i].penalty;
            break;
        }
    }
    if (obj->grade < 0.0) {
        obj->grade = 0.0;
    }
    obj->grade_text = interpret_grade(obj->grade);
}


static TestResultsSummary TestResults_print_report(const TestResults *results,
    size_t ntests, time_t nseconds_total, ReportType rtype)
{
    TestResultsSummary summary;
    TestResultsSummary_fill(&summary, results, ntests);
    if (rtype != REPORT_FULL && summary.npassed == ntests) {
        printf("\n\n"
            "---------------------------------------------------\n"
            "----- All tests have been passed successfully -----\n"
            "---------------------------------------------------\n\n");
    } else {
        printf("  %3s %-20s %12s %14s %-15s %4s\n",
            "#", "Test name", "xemp", "p", "Interpretation", "Thr#");
        print_bar();
        for (size_t i = 0; i < ntests; i++) {
            char pvalue_txt[32];
            PValueCategory pvalue_cat = get_pvalue_category(results[i].p);
            if (rtype == REPORT_FULL || pvalue_cat != PVALUE_PASSED) {
                snprintf_pvalue(pvalue_txt, 32, results[i].p, results[i].alpha);
                printf("  %3u %-20s %12g %14s %-15s %4llu\n",
                    results[i].id, results[i].name, results[i].x, pvalue_txt,
                    interpret_pvalue(results[i].p),
                    (unsigned long long) results[i].thread_id);
            }
        }
        print_bar();
    }
    printf("Passed:        %u\n", summary.npassed);
    printf("Suspicious:    %u\n", summary.nwarnings);
    printf("Failed:        %u\n", summary.nfailed);
    if (ntests >= 5) {
        printf("Quality (0-4): %.2f (%s)\n", summary.grade, summary.grade_text);
    }
    printf("Elapsed time:  ");
    print_elapsed_time((unsigned long long) nseconds_total);
    printf("\n");
    return summary;
}

/**
 * @brief Prints a summary about the test battery to stdout.
 */
void TestsBattery_print_info(const TestsBattery *obj)
{
    size_t ntests = TestsBattery_ntests(obj);
    printf("===== Battery '%s' summary =====\n", obj->name);
    printf("  %3s %-20s\n", "#", "Test name");
    print_bar();
    for (size_t i = 0; i < ntests; i++) {
        printf("  %3d %-20s\n", (int) i + 1, obj->tests[i].name);
    }
    print_bar();
    printf("\n\n");
}

/**
 * @brief Runs the given battery of the statistical test for the given
 * pseudorandom number generator.
 */
BatteryExitCode TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype)
{
    time_t tic, toc;
    size_t ntests = TestsBattery_ntests(bat);
    size_t nresults = ntests;
    TestResults *results = NULL;
    if (nthreads == 0) {
        nthreads = 1;
    }
#ifdef NOTHREADS
    nthreads = 1;
    printf("WARNING: multithreading is not supported on this platform\n");
    printf("They will be run sequentally\n");
#endif
    printf("===== Starting '%s' battery =====\n", bat->name);
    if (testid > ntests) { // testid is 1-based index
        fprintf(stderr, "Invalid test id %u", testid);
        return BATTERY_ERROR;
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
        return BATTERY_ERROR;
    }
    // Create a PRNG example: either for one-threaded version or for basic
    // sanity check for multithreaded version.
    GeneratorState obj = GeneratorState_create(gen, intf);
    if (GeneratorState_check_size(&obj) == 0) {
        GeneratorState_destruct(&obj);
        free(results);
        fprintf(stderr, "***** TestsBattery_run: invalid generator output size *****\n");
        return BATTERY_ERROR;            
    }
    // Run the tests
    tic = time(NULL);
    if (nthreads == 1 || testid != TESTS_ALL) {
        // One-threaded version
        if (testid == TESTS_ALL) {
            for (size_t i = 0; i < ntests; i++) {
                intf->printf("----- Test %u of %u (%s)\n",
                    (unsigned int) (i + 1), (unsigned int) ntests, bat->tests[i].name);
                results[i] = TestDescription_run(&bat->tests[i], &obj);
                results[i].name = bat->tests[i].name;
                results[i].id = (unsigned int) (i + 1);
                results[i].thread_id = 0;
            }
        } else {
            *results = TestDescription_run(&bat->tests[testid - 1], &obj);
            results->name = bat->tests[testid - 1].name;
            results->id = testid;
        }
        GeneratorState_destruct(&obj);
    } else {
        // Multithreaded version
        GeneratorState_destruct(&obj);
        TestsBattery_run_threads(bat, gen, intf, nthreads, results, rtype);
    }
    toc = time(NULL);
    printf("\n");
    if (rtype == REPORT_FULL) {
        printf("==================== Seeds logger report ====================\n");
        Entropy_print_seeds_log(&entropy, stdout);
    }
    if (testid == TESTS_ALL) {
        printf("==================== '%s' battery report ====================\n", bat->name);
    } else {
        printf("==================== '%s' battery test #%d report ====================\n",
            bat->name, testid);
    }
    printf("Generator name:    %s\n", gen->name);
    printf("Output size, bits: %d\n", (int) gen->nbits);
    const uint32_t *seed_key = Entropy_get_key(&entropy);
    char *seed_key_txt = sr_u32_bigendian_to_base64(seed_key, 8);
    if (seed_key_txt != NULL) {
        printf("Used seed:         _%.2X_%s\n\n", nthreads, seed_key_txt);
    } else {
        printf("Used seed:         _%.2X_???\n\n", nthreads);
    }
    TestResultsSummary summary =
        TestResults_print_report(results, nresults, toc - tic, rtype);
    if (seed_key_txt != NULL) {
        printf("Used seed:     _%.2X_%s\n", nthreads, seed_key_txt);
    } else {
        printf("Used seed:     _%.2X_???\n", nthreads);
    }
    free(seed_key_txt);
    free(results);
    return (summary.nfailed == 0) ? BATTERY_PASSED : BATTERY_FAILED;
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

/////////////////////////////////////////////////////////////////
///// Implementationof generator that returns upper 32 bits /////
/////////////////////////////////////////////////////////////////

static uint64_t get_bits64_high32(void *state)
{
    EnvelopedGeneratorState *obj = state;
    uint64_t x = obj->parent_gi->get_bits(obj->parent_state);
    return x >> 32;
}


GeneratorInfo define_high32_generator(const GeneratorInfo *gi)
{
    GeneratorInfo gi_env = *gi;
    gi_env.name = "High32";
    gi_env.parent = gi;
    gi_env.nbits = 32;
    gi_env.create = create_enveloped;
    gi_env.free = free_enveloped;
    gi_env.get_bits = get_bits64_high32;
    gi_env.get_sum = NULL;
    return gi_env;
}

/////////////////////////////////////////////////////////////////
///// Implementationof generator that returns lower 32 bits /////
/////////////////////////////////////////////////////////////////

static uint64_t get_bits64_low32(void *state)
{
    EnvelopedGeneratorState *obj = state;
    uint64_t x = obj->parent_gi->get_bits(obj->parent_state);
    return x & 0xFFFFFFFF;
}


GeneratorInfo define_low32_generator(const GeneratorInfo *gi)
{
    GeneratorInfo gi_env = *gi;
    gi_env.name = "Low32";
    gi_env.parent = gi;
    gi_env.nbits = 32;
    gi_env.create = create_enveloped;
    gi_env.free = free_enveloped;
    gi_env.get_bits = get_bits64_low32;
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
void set_bin_stdout(void)
{
#if defined(USE_LOADLIBRARY) || defined(NO_POSIX)
    (void) _setmode( _fileno(stdout), _O_BINARY);
#endif
}


/**
 * @brief Switches stdin to binary mode in MS Windows (needed for
 * correct input of binary data)
 */
void set_bin_stdin(void)
{
#if defined(USE_LOADLIBRARY) || defined(NO_POSIX)
    (void) _setmode( _fileno(stdin), _O_BINARY);
#endif
}


static inline unsigned int
maxlen_log2_to_nblocks32_log2(unsigned int maxlen_log2)
{
    if (maxlen_log2 == 0) {
        return 62;
    } else if (maxlen_log2 < 11) {
        return 1;
    } else {
        return maxlen_log2 - 10;
    }
}

/**
 * @brief Dump an output PRNG to the stdout in the format suitable
 * for PractRand.
 */
void GeneratorInfo_bits_to_file(GeneratorInfo *gen,
    const CallerAPI *intf, unsigned int maxlen_log2)
{
    set_bin_stdout();
    void *state = gen->create(gen, intf);
    unsigned int shl = maxlen_log2_to_nblocks32_log2(maxlen_log2);
    if (gen->nbits == 32) {
        uint32_t buf[256];        
        long long nblocks = 1ll << shl;
        for (long long i = 0; i < nblocks; i++) {
            for (size_t j = 0; j < 256; j++) {
                buf[j] = (uint32_t) gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint32_t), 256, stdout);
        }
    } else {
        uint64_t buf[256];
        long long nblocks = 1ll << (--shl);
        for (long long i = 0; i < nblocks; i++) {
            for (size_t j = 0; j < 256; j++) {
                buf[j] = gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint64_t), 256, stdout);
        }
    }
}
