#ifndef __SMOKERAND_CINTERFACE_H
#define __SMOKERAND_CINTERFACE_H
#include "smokerand/apidefs.h"

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
#define MAKE_UINT_PRNG(prng_name, selftest_func, numofbits) \
EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); } \
GET_SUM_FUNC \
int EXPORT gen_getinfo(GeneratorInfo *gi) { \
    gi->name = prng_name; \
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
    MAKE_UINT_PRNG(prng_name, selftest_func, 32)

/**
 * @brief Some default boilerplate code for scalar PRNG that returns
 * unsigned 64-bit numbers.
 */
#define MAKE_UINT64_PRNG(prng_name, selftest_func) \
    MAKE_UINT_PRNG(prng_name, selftest_func, 64)


////////////////////////////////////////////////////////////
///// Some useful pre-defined structures and functions /////
////////////////////////////////////////////////////////////

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

/**
 * @brier 128-bit LCG seeding procedure, suitable for MCGs (i.e. for c = 0)
 */
static inline void Lcg128State_seed(Lcg128State *obj, const CallerAPI *intf)
{
#ifdef UINT128_ENABLED
    obj->x = intf->get_seed64() | 0x1; // For MCG
#else
    obj->x_low = intf->get_seed64() | 0x1; // For MCG
    obj->x_high = intf->get_seed64();
#endif
}

#endif
// end of UMUL128_FUNC_ENABLED ifdef


#endif
