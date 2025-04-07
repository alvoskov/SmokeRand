/**
 * @file cinterface.h
 * @brief C interface for modules (dynamic libraries) with pseudorandom
 * number generators implementations.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CINTERFACE_H
#define __SMOKERAND_CINTERFACE_H
#include "smokerand/apidefs.h"

///////////////////////////////
///// 128-bit arithmetics /////
///////////////////////////////

static inline void uadd_128p64_ary_c99(uint32_t *x, uint64_t c)
{
    static const uint64_t MASK32 = 0xFFFFFFFF;
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
    uint32_t out[4], x_lo = b & MASK32, x_hi = b >> 32;
    uint64_t mul, sum;
    uint64_t a_lo = a & MASK32, a_hi = a >> 32;
    // Row 0
    mul = a_lo * x_lo;
    out[0] = mul & MASK32;
    mul = a_lo * x_hi + (mul >> 32);
    out[1] = mul & MASK32; out[2] = mul >> 32;
    // Row 1
    mul = a_hi * x_lo;
    sum = (mul & MASK32) + out[1];
    out[1] = sum & MASK32;
        
    mul = a_hi * x_hi + (mul >> 32);
    sum = (mul & MASK32) + out[2] + (sum >> 32);
    out[2] = sum & MASK32;
    out[3] = (sum >> 32) + (mul >> 32);
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
    static const uint64_t MASK32 = 0xFFFFFFFF;
    uint32_t out[4];
    out[0] = (*a_lo) & MASK32; out[1] = (*a_lo) >> 32;
    out[2] = (*a_hi) & MASK32; out[3] = (*a_hi) >> 32;
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


//////////////////////
///// Interfaces /////
//////////////////////

#define PRNG_CMODULE_PROLOG SHARED_ENTRYPOINT_CODE

static void *create(const CallerAPI *intf);

/**
 * @brief Default create function (constructor) for PRNG. Will call
 * user-defined constructor.
 */
static void *default_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create(intf);
}

/**
 * @brief Default free function (destructor) for PRNG.
 */
static void default_free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    intf->free(state);
}


/**
 * @brief Defines a function that returns a sum of pseudorandom numbers
 * sample. Useful for performance measurements of fast PRNGs.
 */
#define GET_SUM_FUNC EXPORT uint64_t get_sum(void *state, size_t len) { \
    uint64_t sum = 0; \
    for (size_t i = 0; i < len; i++) { \
        sum += get_bits_raw(state); \
    } \
    return sum; \
}

#ifndef GEN_DESCRIPTION
#define GEN_DESCRIPTION NULL
#endif

/**
 * @brief  Some default boilerplate code for scalar PRNG that returns
 * unsigned integers.
 * @details  Requires the next functions to be defined:
 *
 * - `static uint64_t get_bits(void *state);`
 * - `static void *create(CallerAPI *intf);`
 *
 * It also relies on default prolog (intf static variable, some exports etc.),
 * see PRNG_CMODULE_PROLOG
 */
#define MAKE_UINT_PRNG(prng_name, selftest_func, numofbits, get_prng_name_func) \
EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); } \
GET_SUM_FUNC \
int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf) { (void) intf; \
    if (intf != NULL && get_prng_name_func != NULL) { \
        const char *(*getname)(const CallerAPI *) = get_prng_name_func; \
        gi->name = getname(intf); \
    } else { \
        gi->name = prng_name; \
    } \
    gi->description = GEN_DESCRIPTION; \
    gi->nbits = numofbits; \
    gi->get_bits = get_bits; \
    gi->create = default_create; \
    gi->free = default_free; \
    gi->get_sum = get_sum; \
    gi->self_test = selftest_func; \
    gi->parent = NULL; \
    return 1; \
}

/**
 * @brief Some default boilerplate code for scalar PRNG that returns
 * unsigned 32-bit numbers.
 */
#define MAKE_UINT32_PRNG(prng_name, selftest_func) \
    MAKE_UINT_PRNG(prng_name, selftest_func, 32, NULL)

/**
 * @brief Some default boilerplate code for scalar PRNG that returns
 * unsigned 64-bit numbers.
 */
#define MAKE_UINT64_PRNG(prng_name, selftest_func) \
    MAKE_UINT_PRNG(prng_name, selftest_func, 64, NULL)

/**
 * @brief Generates two functions for a user defined `get_bits_SUFFIX_raw`:
 * `get_bits_SUFFIX` and `get_sum_SUFFIX`. Useful for custom modules that
 * contain several variants of the same generator.
 */
#define MAKE_GET_BITS_WRAPPERS(suffix) \
static uint64_t get_bits_##suffix(void *state) { \
    return get_bits_##suffix##_raw(state); \
} \
static uint64_t get_sum_##suffix(void *state, size_t len) { \
    uint64_t sum = 0; \
    for (size_t i = 0; i < len; i++) { \
        sum += get_bits_##suffix##_raw(state); \
    } \
    return sum; \
}

///////////////////////////////////////////////////////
///// Some predefined structures for PRNGs states /////
///////////////////////////////////////////////////////

/**
 * @brief 32-bit LCG state.
 */
typedef struct {
    uint32_t x;
} Lcg32State;

/**
 * @brief 64-bit LCG state.
 */
typedef struct {
    uint64_t x;
} Lcg64State;


///////////////////////////////////////////////////////
///// Structures for PRNGs based on block ciphers /////
///////////////////////////////////////////////////////

/**
 * @brief A generalized interface for both scalar and vectorized versions
 * of the PRNG with buffer. Should be placed at the beginning of the PRNG
 * state.
 * @details Used to emulate inheritance from an abstract class.
 */
typedef struct {    
    void (*iter_func)(void *); ///< Pointer to the block generation function.
    int pos; ///< Current position in the buffer.
    int bufsize; ///< Buffer size in 32-bit words.
    uint32_t *out; ///< Pointer to the output buffer.
} BufGen32Interface;

/**
 * @brief Declares the `get_bits_raw` inline function for the generator
 * that uses the `BufGen32Interface` interface structure.
 */
#define BUFGEN32_DEFINE_GET_BITS_RAW \
static inline uint64_t get_bits_raw(void *state) { \
    BufGen32Interface *obj = state; \
    if (obj->pos >= obj->bufsize) { \
        obj->iter_func(obj); \
    } \
    return obj->out[obj->pos++]; \
}


/**
 * @brief A generalized interface for both scalar and vectorized versions
 * of the PRNG with buffer. Should be placed at the beginning of the PRNG
 * state.
 * @details Used to emulate inheritance from an abstract class.
 */
typedef struct {    
    void (*iter_func)(void *); ///< Pointer to the block generation function.
    int pos; ///< Current position in the buffer.
    int bufsize; ///< Buffer size in 32-bit words.
    uint64_t *out; ///< Pointer to the output buffer.
} BufGen64Interface;

/**
 * @brief Declares the `get_bits_raw` inline function for the generator
 * that uses the `BufGen64Interface` interface structure.
 */
#define BUFGEN64_DEFINE_GET_BITS_RAW \
static inline uint64_t get_bits_raw(void *state) { \
    BufGen64Interface *obj = state; \
    if (obj->pos >= obj->bufsize) { \
        obj->iter_func(obj); \
    } \
    return obj->out[obj->pos++]; \
}

///////////////////////////////////////////////////////////////
///// Structures and functions for 128-bit implementation /////
///////////////////////////////////////////////////////////////

#ifdef UMUL128_FUNC_ENABLED

/**
 * @brief 128-bit LCG state.
 * @details It has two versions: for compilers with 128-bit integers (GCC,Clang)
 * and for MSVC that has no such integers but has some compiler intrinsics
 * for 128-bit multiplication.
 */
typedef struct {
#ifdef UINT128_ENABLED
    unsigned __int128 x;
#else
    uint64_t x_low;
    uint64_t x_high;
#endif
} Lcg128State;


/**
 * @brief A cross-compiler implementation of 128-bit LCG with 64-bit multiplier.
 */
static inline uint64_t Lcg128State_a64_iter(Lcg128State *obj, const uint64_t a, const uint64_t c)
{
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
}


/**
 * @brief A cross-compiler implementation of 128-bit LCG with 128-bit multiplier.
 */
static inline uint64_t Lcg128State_a128_iter(Lcg128State *obj,
    const uint64_t a_high, const uint64_t a_low, const uint64_t c)
{
#ifdef UINT128_ENABLED
    const unsigned __int128 a = ((unsigned __int128) a_high) << 64 | a_low;
    obj->x = a * obj->x + c;
    return (uint64_t) (obj->x >> 64);
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

static inline void Lcg128State_init(Lcg128State *obj, uint64_t hi, uint64_t lo)
{
#ifdef UINT128_ENABLED
    obj->x = hi;
    obj->x <<= 64;
    obj->x |= lo; 
#else
    obj->x_low = lo;
    obj->x_hi  = hi;
#endif
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


#endif
// end of UMUL128_FUNC_ENABLED ifdef


#endif
