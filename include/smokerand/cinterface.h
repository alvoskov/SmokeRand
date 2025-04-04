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
#define unsigned_mul128 \
\
#pragma error "128-bit arithmetics is not supported by this compiler" \
\
//# pragma message ("128-bit arithmetics is not supported by this compiler")
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
