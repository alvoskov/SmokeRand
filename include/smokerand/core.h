/**
 * @file core.h
 * @brief Subroutines and special functions required for implementation
 * of statistical tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CORE_H
#define __SMOKERAND_CORE_H
#include <stdint.h>
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define TESTS_ALL 0

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#pragma intrinsic(_umul128)
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    return _umul128(a, b, high);
}
#else
#define UINT128_ENABLED
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    __uint128_t mul = ((__uint128_t) a) * b;
    *high = mul >> 64;
    return (uint64_t) mul;
}
#endif


#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
#define EXPORT __declspec( dllexport )
#define USE_LOADLIBRARY
#ifndef __cplusplus
#define SHARED_ENTRYPOINT_CODE \
int DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
    (void) hinstDLL; (void) fdwReason; (void) lpvReserved; return TRUE; }
#else
#define SHARED_ENTRYPOINT_CODE
#endif
#else
#include <unistd.h>
#define EXPORT
#define SHARED_ENTRYPOINT_CODE
#endif


#ifdef USE_LOADLIBRARY
#include <windows.h>
#define DLSYM_WRAPPER GetProcAddress
#define DLCLOSE_WRAPPER FreeLibrary
#define MODULE_HANDLE HMODULE
#else
#include <dlfcn.h>
#define DLSYM_WRAPPER dlsym
#define DLCLOSE_WRAPPER dlclose
#define MODULE_HANDLE void *
#endif

int get_cpu_numcores(void);

typedef struct {
    uint32_t (*get_seed32)(void); ///< Get 32-bit seed
    uint64_t (*get_seed64)(void); ///< Get 64-bit seed
    void *(*malloc)(size_t len); ///< Pointer to malloc function
    void (*free)(void *); ///< Pointer to free function
    int (*printf)(const char *format, ... ); ///< Pointer to printf function
    int (*strcmp)(const char *lhs, const char *rhs); ///< Pointer to strcmp function
} CallerAPI;

CallerAPI CallerAPI_init(void);
CallerAPI CallerAPI_init_mthr(void);
void CallerAPI_free(void);

/**
 * @brief Keeps the description of pseudorandon number generator.
 * Either 32-bit or 64-bit PRNGs are supported.
 */
typedef struct {
    const char *name; ///< Generator name
    void *(*create)(const CallerAPI *intf); ///< Create PRNG example
    uint64_t (*get_bits)(void *state); ///< Return u32/u64 pseudorandom number
    unsigned int nbits; ///< Number of bits returned by the generator (32 or 64)    
    int (*self_test)(const CallerAPI *intf); ///< Run internal self-test
    uint64_t (*get_sum)(void *state, size_t len); ///< Return sum of `len` elements
} GeneratorInfo;


typedef int (*GetGenInfoFunc)(GeneratorInfo *gi);

/**
 * @brief Input data for generic statistical test, mainly PNG and its state.
 */
typedef struct {
    const GeneratorInfo *gi; ///< Generator to be tested
    void *state; ///< Pointer to generator state
    const CallerAPI *intf; ///< Will be used for output
} GeneratorState;


typedef struct
{
    MODULE_HANDLE lib;
    int valid;
    GeneratorInfo gen;
} GeneratorModule;

GeneratorModule GeneratorModule_load(const char *libname);
void GeneratorModule_unload(GeneratorModule *mod);


/**
 * @brief Test name and results.
 */
typedef struct {
    const char *name; ///< Test name
    double p; ///< p-value
    double alpha; ///< 1 - p where p is p-value
    double x; ///< Empirical random value
    uint64_t thread_id; ///< Thread ID for logging
} TestResults;

/**
 * @brief Test generalized description.
 */
typedef struct {
    const char *name;
    TestResults (*run)(GeneratorState *obj);
    unsigned int nseconds; ///< Estimated time, seconds
} TestDescription;


/**
 * @brief Tests battery description.
 */
typedef struct {
    const char *name; 
    const TestDescription *tests;
} TestsBattery;

size_t TestsBattery_ntests(const TestsBattery *obj);
void TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads);


typedef enum {
    pvalue_passed = 0,
    pvalue_warning = 1,
    pvalue_failed = 2
} PValueCategory;

const char *interpret_pvalue(double pvalue);
PValueCategory get_pvalue_category(double pvalue);
double ks_pvalue(double x);
double gammainc(double a, double x);
double poisson_cdf(double x, double lambda);
double poisson_pvalue(double x, double lambda);
double chi2_cdf(double x, unsigned long f);
double chi2_pvalue(double x, unsigned long f);
void radixsort32(uint32_t *x, size_t len);
void radixsort64(uint64_t *x, size_t len);

typedef struct {
    void *original_state;
    const GeneratorInfo *original_gen;
} ReversedGen32State;


GeneratorInfo reversed_generator_set(const GeneratorInfo *gi);

typedef struct {
    unsigned int h; ///< Hours
    unsigned short m; ///< Minutes
    unsigned short s; ///< Seconds
} TimeHMS;


TimeHMS nseconds_to_hms(unsigned long nseconds_total);
void print_elapsed_time(unsigned long nseconds_total);

////////////////////////////////////////
///// Some useful inline functions /////
////////////////////////////////////////

static inline uint32_t reverse_bits32(uint32_t x)
{
    x = (x >> 16) | (x << 16);
    x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
    x = ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4);
    x = ((x & 0xCCCCCCCC) >> 2) | ((x & 0x33333333) << 2);
    x = ((x & 0xAAAAAAAA) >> 1) | ((x & 0x55555555) << 1);
    return x;
}


static inline uint64_t reverse_bits64(uint64_t x)
{
    x = (x >> 32) | (x << 32);
    x = ((x & 0xFFFF0000FFFF0000) >> 16) | ((x & 0x0000FFFF0000FFFF) << 16);
    x = ((x & 0xFF00FF00FF00FF00) >> 8)  | ((x & 0x00FF00FF00FF00FF) << 8);
    x = ((x & 0xF0F0F0F0F0F0F0F0) >> 4)  | ((x & 0x0F0F0F0F0F0F0F0F) << 4);
    x = ((x & 0xCCCCCCCCCCCCCCCC) >> 2)  | ((x & 0x3333333333333333) << 2);
    x = ((x & 0xAAAAAAAAAAAAAAAA) >> 1)  | ((x & 0x5555555555555555) << 1);
    return x;
}




/**
 * @brief pcg_rxs_m_xs64 PRNG that has a good quality and can be used
 * for initialization for other PRNGs such as lagged Fibonacci.
 */
static inline uint64_t pcg_bits64(uint64_t *state)
{
    uint64_t word = ((*state >> ((*state >> 59) + 5)) ^ *state) *
        12605985483714917081ull;
    *state = *state * 6364136223846793005ull + 1442695040888963407ull;
    return (word >> 43) ^ word;
}

#endif
