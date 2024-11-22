/**
 * @file apidefs.h
 * @brief Data types and definitions required for PRNG C interface.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_APIDEFS_H
#define __SMOKERAND_APIDEFS_H
#include <stdint.h>
#include <stddef.h>

///////////////////////////////
///// 128-bit arithmetics /////
///////////////////////////////

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#define UMUL128_FUNC_ENABLED
#pragma intrinsic(_umul128)
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    return _umul128(a, b, high);
}
#elif defined(__SIZEOF_INT128__)
#define UINT128_ENABLED
#define UMUL128_FUNC_ENABLED
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    __uint128_t mul = ((__uint128_t) a) * b;
    *high = mul >> 64;
    return (uint64_t) mul;
}
#else
#pragma message ("128-bit arithmetics is not supported by this compiler")
#endif


//////////////////////////////////////////
///// Custom DLL entry point for GCC /////
//////////////////////////////////////////

#if !defined(__cplusplus) && (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(__clang__)
#include <windows.h>
#define SHARED_ENTRYPOINT_CODE \
int DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
    (void) hinstDLL; (void) fdwReason; (void) lpvReserved; return TRUE; }
#else
#define SHARED_ENTRYPOINT_CODE
#endif

//////////////////////////////
///// API for generators /////
//////////////////////////////

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif


typedef struct {
    uint32_t (*get_seed32)(void); ///< Get 32-bit seed
    uint64_t (*get_seed64)(void); ///< Get 64-bit seed
    const char *(*get_param)(void); ///< Get command line parameters
    void *(*malloc)(size_t len); ///< Pointer to malloc function
    void (*free)(void *); ///< Pointer to free function
    int (*printf)(const char *format, ... ); ///< Pointer to printf function
    int (*strcmp)(const char *lhs, const char *rhs); ///< Pointer to strcmp function
} CallerAPI;


/**
 * @brief Keeps the description of pseudorandon number generator.
 * Either 32-bit or 64-bit PRNGs are supported.
 */
typedef struct {
    const char *name; ///< Generator name
    const char *description; ///< Generator description (optional)
    void *(*create)(const CallerAPI *intf); ///< Create PRNG example
    uint64_t (*get_bits)(void *state); ///< Return u32/u64 pseudorandom number
    unsigned int nbits; ///< Number of bits returned by the generator (32 or 64)    
    int (*self_test)(const CallerAPI *intf); ///< Run internal self-test
    uint64_t (*get_sum)(void *state, size_t len); ///< Return sum of `len` elements
} GeneratorInfo;


typedef int (*GetGenInfoFunc)(GeneratorInfo *gi);


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
