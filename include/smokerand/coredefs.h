/**
 * @file coredefs.h
 * @brief Some basic definitions and declarations used in different parts
 * of SmokeRand test suite.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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

#endif // __SMOKERAND_COREDEFS_H
