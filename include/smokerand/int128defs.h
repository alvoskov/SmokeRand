/**
 * @file int128defs.h
 * @brief Implementation of 128-bit arithmetics subset required for linear
 * congruential generators (LCGs). Tries to use efficient compiler-specific
 * data types, intrinstics. Also has cross-platform C99 specific versions
 * of functions, mainly for 32-bit platforms.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_INT128DEFS_H
#define __SMOKERAND_INT128DEFS_H
#include <stdint.h>
#include <stddef.h>

///////////////////////////////////////////////////
///// 128-bit arithmetics: portable functions /////
///////////////////////////////////////////////////

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
    uint32_t out[4], x_lo = (uint32_t) b, x_hi = (uint32_t) (b >> 32);
    uint64_t mul, sum;
    uint64_t a_lo = a & MASK32, a_hi = (uint32_t) (a >> 32);
    // Row 0
    mul = a_lo * x_lo;
    out[0] = (uint32_t) mul;
    mul = a_lo * x_hi + (mul >> 32);
    out[1] = (uint32_t) mul; out[2] = (uint32_t) (mul >> 32);
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
    out[0] = (uint32_t) (*a_lo); out[1] = (uint32_t) ((*a_lo) >> 32);
    out[2] = (uint32_t) (*a_hi); out[3] = (uint32_t) ((*a_hi) >> 32);
    uadd_128p64_ary_c99(out, b);
    (*a_hi) = ((uint64_t) out[2]) | (((uint64_t) out[3]) << 32);
    (*a_lo) = ((uint64_t) out[0]) | (((uint64_t) out[1]) << 32);
}

/**
 * @brief A portable implementation of the \f$ ax + c \f$ operation with 64-bit
 * arguments and 128-bit output. Useful for LCG and MWC generators.
 * @details
 *
 *          |x x x x     x * a[0]
 *     +   x|x x x       x * a[1]
 *       x x|x x         x * a[2]
 *     x x x|x           x * a[3]
 *     --------------
 *          |x x x x
 *
 * @param[in] a Input value.
 * @param[in,out] x Input value that will be overwritten.
 * @param[in] c Input value.
 * @return Lower 64 bits.
 */
static inline void umuladd_128x128p64_c99(const uint32_t *a, uint32_t *x, uint64_t c)
{
    static const uint64_t MASK32 = 0xFFFFFFFF;
    uint32_t out[4];
    uint64_t mul, sum;
    // Row 0
    mul = 0;
    for (int i = 0; i < 4; i++) {
        mul = ((uint64_t) a[0]) * x[i] + (mul >> 32);
        out[i] = (uint32_t) mul;
    }
    // Row 1
    mul = 0, sum = 0;
    for (int i = 0; i < 3; i++) {
        mul = ((uint64_t) a[1]) * x[i] + (mul >> 32);
        sum = (mul & MASK32) + out[i + 1] + (sum >> 32);
        out[i + 1] = (uint32_t) sum;
    }
    // Row 2
    mul = 0; sum = 0;
    for (int i = 0; i < 2; i++) {
        mul = ((uint64_t) a[2]) * x[i] + (mul >> 32);
        sum = (mul & MASK32) + out[i + 2] + (sum >> 32);
        out[i + 2] = (uint32_t) sum;
    }
    // Row 3
    out[3] += a[3] * x[0];
    if (c != 0) {
        uadd_128p64_ary_c99(out, c);
    }
    // Update output
    for (int i = 0; i < 4; i++) {
        x[i] = out[i];
    }
}


////////////////////////////////////////////////////////////////
///// 128-bit arithmetics: compiler-specific optimizations /////
////////////////////////////////////////////////////////////////


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

static inline void umuladd_128x128p64w(uint64_t a_hi, uint64_t a_lo,
    uint64_t *x_hi, uint64_t *x_lo, uint64_t c)
{
    // Process lower part of a
    uint64_t mul0_high, x_low_old = *x_lo;
    *x_lo = _umul128(a_lo, *x_lo, &mul0_high);
    *x_hi = a_lo * (*x_hi) + mul0_high;
    // Process higher part of a
    *x_hi += a_hi * x_low_old;
    // Add constant
    *x_hi += _addcarry_u64(0, *x_lo, c, x_lo);
}

// End of MSVC implementation of 128-bit arithmetics
#elif defined(__SIZEOF_INT128__)
// Begin of GCC implementation of 128-bit arithmetics
#define UINT128_ENABLED
#define UMUL128_FUNC_ENABLED
static inline uint64_t unsigned_mul128(uint64_t a, uint64_t b, uint64_t *high)
{
    __uint128_t mul = ((__uint128_t) a) * b;
    *high = (uint64_t) (mul >> 64);
    return (uint64_t) mul;
}

static inline uint64_t unsigned_muladd128(uint64_t a, uint64_t b, uint64_t c, uint64_t *high)
{
    const __uint128_t t = a * ((__uint128_t) b) + c;
    *high = (uint64_t) (t >> 64);
    return (uint64_t) t;
}

static inline void unsigned_add128(uint64_t *a_hi, uint64_t *a_lo, uint64_t b)
{
    const __uint128_t t = (((__uint128_t)(*a_hi) << 64) | (*a_lo)) + b;
    *a_lo = (uint64_t) t;
    *a_hi = (uint64_t) (t >> 64);
}

static inline void umuladd_128x128p64w(uint64_t a_hi, uint64_t a_lo,
    uint64_t *x_hi, uint64_t *x_lo, uint64_t c)
{
    const __uint128_t a = ((__uint128_t) a_hi) << 64 | a_lo;
    const __uint128_t x = ((__uint128_t) (*x_hi)) << 64 | (*x_lo);
    __uint128_t t = a * x + c;
    *x_lo = (uint64_t) t;
    *x_hi = (uint64_t) (t >> 64);
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

static inline void umuladd_128x128p64w(uint64_t a_hi, uint64_t a_lo,
    uint64_t *x_hi, uint64_t *x_lo, uint64_t c)
{
    uint32_t x[4], a[4];
    x[0] = (uint32_t) (*x_lo); x[1] = (*x_lo) >> 32;
    x[2] = (uint32_t) (*x_hi); x[3] = (*x_hi) >> 32;
    a[0] = (uint32_t) a_lo; a[1] = a_lo >> 32;
    a[2] = (uint32_t) a_hi; a[3] = a_hi >> 32;
    umuladd_128x128p64_c99(a, x, c);
    *x_hi = ((uint64_t) x[2]) | (((uint64_t) x[3]) << 32);
    *x_lo  = ((uint64_t) x[0]) | (((uint64_t) x[1]) << 32);
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
    uint64_t x_low;
    uint64_t x_high;
} Lcg128State;


static inline void Lcg128State_init(Lcg128State *obj, uint64_t hi, uint64_t lo)
{
    obj->x_low  = lo;
    obj->x_high = hi;
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
}


/**
 * @brief A cross-platform implementation of 128-bit LCG with 128-bit multiplier.
 */
static inline uint64_t Lcg128State_a128_iter(Lcg128State *obj,
    const uint64_t a_high, const uint64_t a_low, const uint64_t c)
{
    umuladd_128x128p64w(a_high, a_low, &obj->x_high, &obj->x_low, c);
    return obj->x_high;
}

#endif // __SMOKERAND_INT128DEFS_H
