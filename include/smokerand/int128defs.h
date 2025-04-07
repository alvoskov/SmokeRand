/**
 * @file int128defs.h
 * @brief Implementation of 128-bit arithmetics subset required for linear
 * congruential generators (LCGs). Tries to use efficient compiler-specific
 * data types, intrinstics. Also has cross-platform C99 specific versions
 * of functions, mainly for 32-bit platforms.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_INT128DEFS_H
#define __SMOKERAND_INT128DEFS_H
#include <stdint.h>
#include <stddef.h>

///////////////////////////////
///// 128-bit arithmetics /////
///////////////////////////////

static inline void uadd_128p64_ary_c99(uint32_t *x, uint64_t c)
{
    static const uint32_t MASK32 = 0xFFFFFFFF;
    uint64_t sum = x[0] + (c & MASK32);     x[0] = sum & MASK32;
    sum = x[1] + ((c >> 32) + (sum >> 32)); x[1] = sum & MASK32;
    sum = x[2] + (sum >> 32);               x[2] = sum & MASK32;
    sum = x[3] + (sum >> 32);               x[3] = sum & MASK32;
}

/**
 * @brief A portable implementation of the \f$ ab + c \f$ operation with 64-bit
 * arguments and 128-bit output. Useful for LCG and MWC generators.
 * @param[in] a Input value.
 * @param[in] b Input value.
 * @param[in] c Input value.
 * @param[out] hi Pointer to the buffer for upper 64 bits.
 * @return Lower 64 bits.
 */
static inline uint64_t umuladd_64x64p64_c99(uint64_t a, uint64_t b, uint64_t c, uint64_t *hi)
{
    static const uint64_t MASK32 = 0xFFFFFFFF;
    uint32_t out[4], x_lo = (uint32_t) b, x_hi = b >> 32;
    uint64_t mul, sum;
    uint64_t a_lo = a & MASK32, a_hi = a >> 32;
    // Row 0
    mul = a_lo * x_lo;
    out[0] = (uint32_t) mul;
    mul = a_lo * x_hi + (mul >> 32);
    out[1] = (uint32_t) mul; out[2] = mul >> 32;
    // Row 1
    mul = a_hi * x_lo;
    sum = (mul & MASK32) + out[1];
    out[1] = (uint32_t) sum;
        
    mul = a_hi * x_hi + (mul >> 32);
    sum = (mul & MASK32) + out[2] + (sum >> 32);
    out[2] = (uint32_t) sum;
    out[3] = (uint32_t) ((sum >> 32) + (mul >> 32));
    if (c != 0) {
        uadd_128p64_ary_c99(out, c);
    }
    (*hi) = ((uint64_t) out[2]) | (((uint64_t) out[3]) << 32);
    return ((uint64_t) out[0]) | (((uint64_t) out[1]) << 32);
}

/**
 * @brief A portable implementation of the `a += b` operation with 128-bit
 * `a` and 64-bit `b`. Useful for LCG and MWC generators.
 */
static inline void uadd_128p64_c99(uint64_t *a_hi, uint64_t *a_lo, uint64_t b)
{
    uint32_t out[4];
    out[0] = (uint32_t) (*a_lo); out[1] = (*a_lo) >> 32;
    out[2] = (uint32_t) (*a_hi); out[3] = (*a_hi) >> 32;
    uadd_128p64_ary_c99(out, b);
    (*a_hi) = ((uint64_t) out[2]) | (((uint64_t) out[3]) << 32);
    (*a_lo) = ((uint64_t) out[0]) | (((uint64_t) out[1]) << 32);
}

#if defined(_MSC_VER) && !defined(__clang__)
// Begin of MSVC implementation of 128-bit arithmetics
#include <intrin.h>
#define UMUL128_FUNC_ENABLED
#pragma intrinsic(_umul128)
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    return _umul128(a, b, high);
}

static inline uint64_t unsigned_muladd128(uint64_t a, uint64_t b, uint64_t c, uint64_t *high)
{
    uint64_t mul = _umul128(a, b, high), low;
    *high += _addcarry_u64(0, mul, c, &low);
    return low;
}

static inline void unsigned_add128(uint64_t *a_hi, uint64_t *a_lo, uint64_t b)
{
    *a_hi += _addcarry_u64(0, *a_lo, b, a_lo);
}

// End of MSVC implementation of 128-bit arithmetics
#elif defined(__SIZEOF_INT128__)
// Begin of GCC implementation of 128-bit arithmetics
#define UINT128_ENABLED
#define UMUL128_FUNC_ENABLED
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    __uint128_t mul = ((__uint128_t) a) * b;
    *high = mul >> 64;
    return (uint64_t) mul;
}

static inline uint64_t unsigned_muladd128(uint64_t a, uint64_t b, uint64_t c, uint64_t *high)
{
    const __uint128_t t = a * ((__uint128_t) b) + c;
    *high = t >> 64;
    return (uint64_t) t;
}

static inline void unsigned_add128(uint64_t *a_hi, uint64_t *a_lo, uint64_t b)
{
    const __uint128_t t = (((__uint128_t)(*a_hi) << 64) | (*a_lo)) + b;
    *a_lo = (uint64_t) t;
    *a_hi = (uint64_t) (t >> 64);
}
// End of GCC implementation of 128-bit arithmetics
#else
// Begin of portable (C99) implementation of 128-bit arithmetics
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    return umuladd_64x64p64_c99(a, b, 0, high);
}

static inline uint64_t unsigned_muladd128(uint64_t a, uint64_t b, uint64_t c, uint64_t *high)
{
    return umuladd_64x64p64_c99(a, b, c, high);
}

static inline void unsigned_add128(uint64_t *a_hi, uint64_t *a_lo, uint64_t b)
{
    uadd_128p64_c99(a_hi, a_lo, b);
}
// End of portable (C99) implementation of 128-bit arithmetics
#endif


///////////////////////////////////////////////////////////////
///// Structures and functions for 128-bit implementation /////
///////////////////////////////////////////////////////////////

/**
 * @brief 128-bit LCG state.
 * @details It has two versions: for compilers with 128-bit integers (GCC,Clang)
 * and for MSVC that has no such integers but has some compiler intrinsics
 * for 128-bit multiplication.
 */
typedef struct {
//#ifdef UINT128_ENABLED
//    unsigned __int128 x;
//#else
    uint64_t x_low;
    uint64_t x_high;
//#endif
} Lcg128State;


static inline void Lcg128State_init(Lcg128State *obj, uint64_t hi, uint64_t lo)
{
//#ifdef UINT128_ENABLED
//    obj->x = hi;
//    obj->x <<= 64;
//    obj->x |= lo;
//#else
    obj->x_low  = lo;
    obj->x_high = hi;
//#endif
}

/**
 * @brier 128-bit LCG seeding procedure, suitable for MCGs (i.e. for c = 0)
 */
static inline void Lcg128State_seed(Lcg128State *obj, const CallerAPI *intf)
{
    uint64_t hi = intf->get_seed64();
    uint64_t lo = intf->get_seed64() | 0x1; // For MCG
    Lcg128State_init(obj, hi, lo);
}

/**
 * @brief A cross-platform implementation of 128-bit LCG with 64-bit multiplier.
 */
static inline uint64_t Lcg128State_a64_iter(Lcg128State *obj, const uint64_t a, const uint64_t c)
{
    uint64_t mul0_high;
    obj->x_low = unsigned_mul128(a, obj->x_low, &mul0_high);
    obj->x_high = a * obj->x_high + mul0_high;
    unsigned_add128(&obj->x_high, &obj->x_low, c);    
    return obj->x_high;
/*
#ifdef UINT128_ENABLED
    obj->x = a * obj->x + c; 
    return (uint64_t) (obj->x >> 64);
#else
    uint64_t mul0_high;
    obj->x_low = unsigned_mul128(a, obj->x_low, &mul0_high);
    obj->x_high = a * obj->x_high + mul0_high;
    obj->x_high += _addcarry_u64(0, obj->x_low, c, &obj->x_low);
    return obj->x_high;
#endif
*/
}



#ifdef UMUL128_FUNC_ENABLED


/**
 * @brief A cross-compiler implementation of 128-bit LCG with 128-bit multiplier.
 */
static inline uint64_t Lcg128State_a128_iter(Lcg128State *obj,
    const uint64_t a_high, const uint64_t a_low, const uint64_t c)
{
#ifdef UINT128_ENABLED
/*
    const unsigned __int128 a = ((unsigned __int128) a_high) << 64 | a_low;
    obj->x = a * obj->x + c;
    return (uint64_t) (obj->x >> 64);
*/
    (void) obj; (void) a_high; (void) a_low; (void) c;
    return 0;
#else
    // Process lower part of a
    uint64_t mul0_high, x_low_old = obj->x_low;
    obj->x_low = unsigned_mul128(a_low, obj->x_low, &mul0_high);
    obj->x_high = a_low * obj->x_high + mul0_high;
    // Process higher part of a
    obj->x_high += a_high * x_low_old;
    // Add constant
    obj->x_high += _addcarry_u64(0, obj->x_low, c, &obj->x_low);
    // Return upper 64 bits
    return obj->x_high;
#endif
}




#endif
// end of UMUL128_FUNC_ENABLED ifdef



#endif