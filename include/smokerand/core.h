/**
 * @file core.h
 * @brief Subroutines and special functions required for implementation
 * of statistical tests.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CORE_H
#define __SMOKERAND_CORE_H
#include "apidefs.h"
#include <stdint.h>

#define PENALTY_FREQ           4.0
#define PENALTY_GAP            4.0
#define PENALTY_ISING2D        4.0
#define PENALTY_BSPACE         3.0
#define PENALTY_COLLOVER       3.0
#define PENALTY_MOD3           2.0
#define PENALTY_SUMCOLLECTOR   2.0
#define PENALTY_GAP16_COUNT0   2.0
#define PENALTY_HAMMING_DISTR  2.0
#define PENALTY_HAMMING_OT     2.0
#define PENALTY_BSPACE_DEC     1.0
#define PENALTY_MATRIXRANK     0.25
#define PENALTY_MATRIXRANK_LOW 0.25
#define PENALTY_LINEARCOMP     0.25

enum {
    TESTS_ALL = 0
};

CallerAPI CallerAPI_init(void);
CallerAPI CallerAPI_init_mthr(void);
void CallerAPI_free(void);
void set_cmd_param(const char *param);
void set_entropy_textseed(const char *seed, size_t len);
void set_use_stderr_for_printf(int val);

/**
 * @brief Input data for generic statistical test, mainly PNG and its state.
 */
typedef struct {
    const GeneratorInfo *gi; ///< Generator to be tested
    void *state; ///< Pointer to generator state
    const CallerAPI *intf; ///< Will be used for output
} GeneratorState;

GeneratorState GeneratorState_create(const GeneratorInfo *gi,
    const CallerAPI *intf);
void GeneratorInfo_print(const GeneratorInfo *gi, int to_stderr);
void GeneratorState_destruct(GeneratorState *obj, const CallerAPI *intf);
int GeneratorState_check_size(const GeneratorState *obj);

typedef struct
{
    void *lib;
    int valid;
    GeneratorInfo gen;
} GeneratorModule;

GeneratorModule GeneratorModule_load(const char *libname, const CallerAPI *intf);
void GeneratorModule_unload(GeneratorModule *mod);


/**
 * @brief Test name and results.
 */
typedef struct {
    const char *name; ///< Test name
    unsigned int id; ///< Test identifier
    double p; ///< p-value
    double alpha; ///< 1 - p where p is p-value
    double x; ///< Empirical random value
    double penalty; ///< Penalty score for failure
    uint64_t thread_id; ///< Thread ID for logging
} TestResults;


TestResults TestResults_create(const char *name);

typedef enum {
    REPORT_BRIEF = 0,
    REPORT_FULL = 1
} ReportType;

/**
 * @brief Exit codes for battries
 */
typedef enum {
    BATTERY_PASSED  = 0, ///< All battery tests have been passed
    BATTERY_FAILED  = 1, ///< Some battery tests failed
    BATTERY_ERROR   = 2, ///< Error during battery run
    BATTERY_UNKNOWN = 3  ///< Unknown battery name
} BatteryExitCode;


/**
 * @brief Test generalized description.
 */
typedef struct {
    const char *name;
    TestResults (*run)(GeneratorState *obj, const void *udata);
    const void *udata; ///< User data for the function
} TestDescription;


static inline TestResults TestDescription_run(const TestDescription *obj, GeneratorState *gs)
{
    return obj->run(gs, obj->udata);
}

/**
 * @brief Tests battery description.
 */
typedef struct {
    const char *name;
    const TestDescription *tests;
} TestsBattery;

size_t TestsBattery_ntests(const TestsBattery *obj);
void TestsBattery_print_info(const TestsBattery *obj);
BatteryExitCode TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype);


typedef enum {
    PVALUE_PASSED = 0,
    PVALUE_WARNING = 1,
    PVALUE_FAILED = 2
} PValueCategory;

const char *interpret_pvalue(double pvalue);
PValueCategory get_pvalue_category(double pvalue);
void quicksort64(uint64_t *x, size_t len);
void radixsort32(uint32_t *x, size_t len);
void radixsort64(uint64_t *x, size_t len);
void fastsort64(const RamInfo *info, uint64_t *x, size_t len);


typedef struct {
    void *original_state;
    const GeneratorInfo *original_gen;
} ReversedGen32State;


GeneratorInfo define_reversed_generator(const GeneratorInfo *gi);
GeneratorInfo define_interleaved_generator(const GeneratorInfo *gi);
GeneratorInfo define_high32_generator(const GeneratorInfo *gi);
GeneratorInfo define_low32_generator(const GeneratorInfo *gi);


typedef struct {
    unsigned int h; ///< Hours
    unsigned short m; ///< Minutes
    unsigned short s; ///< Seconds
} TimeHMS;


TimeHMS nseconds_to_hms(unsigned long long nseconds_total);
void print_elapsed_time(unsigned long long nseconds_total);

void set_bin_stdout(void);
void set_bin_stdin(void);
void GeneratorInfo_bits_to_file(GeneratorInfo *gen,
    const CallerAPI *intf, unsigned int maxlen_log2);

////////////////////////////////////////
///// Some useful inline functions /////
////////////////////////////////////////

/**
 * @brief Calculates the \f$ (O_i - E_i)^2 / E_i \f term.
 * @param Oi  Observed frequency.
 * @param Ei  Expected (theoretical) frequency.
 */
static inline double calc_chi2emp_term(unsigned long long Oi, double Ei)
{
    double delta = (double) Oi - Ei;
    return delta * delta / Ei;
}

/**
 * @brief Calculate Hamming weight (number of 1's for byte)
 * @details The used table can be easily obtained by the next code:
 *
 *     uint8_t *create_bytes_hamming_weights()
 *     {
 *         uint8_t *hw = calloc(256, sizeof(uint8_t));
 *         for (int i = 0; i < 256; i++) {        
 *             for (int j = 0; j < 8; j++) {
 *                 if (i & (1 << j)) {
 *                     hw[i]++;
 *             }
 *         }
 *         return hw;
 *     }
 */
static inline uint8_t get_byte_hamming_weight(uint8_t x)
{
    static const uint8_t hw[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };
    return hw[x];
}

static inline uint8_t get_uint64_hamming_weight(uint64_t x)
{
#if defined(__GNUC__)
    // GCC specific extension (may use a single CPU instruction)
    return (uint8_t) __builtin_popcountll(x);
#else
    // Portable implementation of Hamming weights computation
    uint8_t hw = 0;
    for (int i = 0; i < 8; i++) {
        hw += get_byte_hamming_weight((uint8_t) (x & 0xFF));
        x >>= 8;
    }
    return hw;
#endif
}


static inline uint8_t reverse_bits4(uint8_t x)
{
    x = (uint8_t) ( (x >> 2) | (x << 2) );
    x = (uint8_t) ( ((x & 0xA) >> 1) | ((x & 0x5) << 1) );
    return x;
}

static inline uint8_t reverse_bits8(uint8_t x)
{
    x = (uint8_t) ( (x >> 4) | (x << 4) );
    x = (uint8_t) ( ((x & 0xCC) >> 2) | ((x & 0x33) << 2) );
    x = (uint8_t) ( ((x & 0xAA) >> 1) | ((x & 0x55) << 1) );
    return x;
}


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

#endif // __SMOKERAND_CORE_H
