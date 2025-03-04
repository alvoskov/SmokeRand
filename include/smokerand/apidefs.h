/**
 * @file apidefs.h
 * @brief Data types and definitions required for PRNG C interface.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_APIDEFS_H
#define __SMOKERAND_APIDEFS_H
#include <stdint.h>
#include <stddef.h>
#ifdef __WATCOMC__
#include <stdlib.h>
#endif

///////////////////////////
///// Circular shifts /////
///////////////////////////


static inline uint32_t rotl32(uint32_t x, unsigned int r)
{
    return (x << r) | (x >> ((-r) & 31));
}

static inline uint32_t rotr32(uint32_t x, unsigned int r)
{
    return (x << ((-r) & 31)) | (x >> r);
}

static inline uint64_t rotl64(uint64_t x, unsigned int r)
{
    return (x << r) | (x >> ((-r) & 63));
}

static inline uint64_t rotr64(uint64_t x, unsigned int r)
{
    return (x << ((-r) & 63)) | (x >> r);
}


//////////////////////////////////////////
///// Custom DLL entry point for GCC /////
//////////////////////////////////////////

#if !defined(__cplusplus) && (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(__clang__)
#include <windows.h>

#ifndef NO_CUSTOM_DLLENTRY
#define SHARED_ENTRYPOINT_CODE \
int DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
    (void) hinstDLL; (void) fdwReason; (void) lpvReserved; return TRUE; }
#else
#define SHARED_ENTRYPOINT_CODE
#endif

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


/**
 * @brief Pointers to the API functions used by a caller, i.e. PRNG
 * implementation. Gives access to a seed generator and some subset
 * of C standard library.
 */
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
typedef struct GeneratorInfo_ {
    const char *name; ///< Generator name
    const char *description; ///< Generator description (optional)
    unsigned int nbits; ///< Number of bits returned by the generator (32 or 64)
    void *(*create)(const struct GeneratorInfo_ *gi, const CallerAPI *intf); ///< Create PRNG example
    void (*free)(void *state, const struct GeneratorInfo_ *gi, const CallerAPI *intf); ///< Destroy PRNG example
    uint64_t (*get_bits)(void *state); ///< Return u32/u64 pseudorandom number
    int (*self_test)(const CallerAPI *intf); ///< Run internal self-test
    uint64_t (*get_sum)(void *state, size_t len); ///< Return sum of `len` elements
    const struct GeneratorInfo_ *parent; ///< Used by create/free functions in enveloped generators.
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
