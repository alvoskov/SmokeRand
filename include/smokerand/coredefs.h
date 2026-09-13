/**
 * @file coredefs.h
 * @brief Some basic definitions and declarations used in different parts
 * of SmokeRand test suite.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#ifndef __SMOKERAND_COREDEFS_H
#define __SMOKERAND_COREDEFS_H

#include <stdint.h>
#include <stddef.h>
#ifdef __WATCOMC__
#include <stdlib.h>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

//////////////////////////////////////////
///// Some data types for interfaces /////
//////////////////////////////////////////

#define RAM_SIZE_UNKNOWN -1

/**
 * @brief Keeps information about available and total amounts of physical RAM.
 * May be useful for sorting algorithms tuning.
 */
typedef struct {
    long long phys_total_nbytes; ///< Total number of bytes in physical RAM (or -1 if unknown)
    long long phys_avail_nbytes; ///< Available number of bytes in physical RAM (or -1 if unknown)
} RamInfo;

///////////////////////////
///// Circular shifts /////
///////////////////////////

static inline uint8_t rotl8(uint8_t x, int r)
{
    return (uint8_t) ( (x << r) | (x >> ((-r) & 7)) );
}


static inline uint8_t rotr8(uint8_t x, int r)
{
    return (uint8_t) ( (x << ((-r) & 7)) | (x >> r) );
}

static inline uint16_t rotl16(uint16_t x, int r)
{
    return (uint16_t) ( (x << r) | (x >> ((-r) & 15)) );
}

static inline uint32_t rotl32(uint32_t x, int r)
{
#ifdef __WATCOMC__
    return _lrotl(x, r);
#else
    return (x << r) | (x >> ((-r) & 31));
#endif
}

static inline uint32_t rotr32(uint32_t x, int r)
{
#ifdef __WATCOMC__
    return _lrotr(x, r);
#else
    return (x << ((-r) & 31)) | (x >> r);
#endif
}

static inline uint64_t rotl64(uint64_t x, int r)
{
    return (x << r) | (x >> ((-r) & 63));
}

static inline uint64_t rotr64(uint64_t x, int r)
{
    return (x << ((-r) & 63)) | (x >> r);
}

/////////////////////////////////////////////////
///// Some functions for SWB/AWC generators /////
/////////////////////////////////////////////////

#ifdef __has_builtin
    #if __has_builtin(__builtin_subcll)
        #define HAS_BUILTIN_SUBCLL 1
    #endif
#endif

/**
 * @brief A portable implementation of subtract with borrow operator
 * for 64-bit unsigned integers.
 * @details It is defined the next way:
 *
 *     d = x - y - b_in
 *     res, b_out = d % 2**64, 1 if d < 0 else 0
 */
static inline uint64_t
swb_u64(uint64_t x, uint64_t y, uint64_t b_in, uint64_t *b_out)
{
#if defined(HAS_BUILTIN_SUBCLL)
    unsigned long long b_out_buf;
    const uint64_t ans = __builtin_subcll(x, y, b_in, &b_out_buf);
    *b_out = (uint64_t) b_out_buf & 0x1;
    return ans;
#elif defined(__GNUC__) && (__GNUC__ >= 5)
    uint64_t d1, d2;
    const int of1 = __builtin_sub_overflow(x, y, &d1);
    const int of2 = __builtin_sub_overflow(d1, b_in, &d2);
    *b_out = of1 || of2;
    return d2;
#elif defined(_MSC_VER) && defined(_WIN64)
    uint64_t ans;
    *b_out = _subborrow_u64((unsigned char) b_in, x, y, &ans);
    return ans;
#else
    const uint64_t d1 = x - y;
    const uint64_t d2 = d1 - b_in;
    *b_out = (x < y) || (d1 < b_in);
    return d2;
#endif
}


#endif // __SMOKERAND_COREDEFS_H
