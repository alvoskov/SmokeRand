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
#include "smokerand/int128defs.h"

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

#endif
