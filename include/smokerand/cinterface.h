/**
 * @file cinterface.h
 * @brief C interface for modules (dynamic libraries) with pseudorandom
 * number generators implementations.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CINTERFACE_H
#define __SMOKERAND_CINTERFACE_H
#include "smokerand/apidefs.h"

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
#define MAKE_UINT_PRNG(prng_name, selftest_func, numofbits) \
EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); } \
GET_SUM_FUNC \
int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf) { (void) intf; \
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

///////////////////////////////////////////////////////////////////////////
///// Some data structures and subroutines for command line interface /////
///////////////////////////////////////////////////////////////////////////

/**
 * @brief Pseudorandom number generator variant description for the given
 * `--param=value` variant.
 */
typedef struct {
    const char *param; ///< Value for the `--param=value` command line option.
    const char *name;  ///< Generator name
    unsigned int nbits; ///< Number of bits in the output (32 or 64)
    void *(*create)(const GeneratorInfo *gi, const CallerAPI *intf);
    uint64_t (*get_bits)(void *state);
    uint64_t (*get_sum)(void *state, size_t len);
} GeneratorParamVariant;

/**
 * @brief Find the PRNG variant by the `--param=value` command line argument.
 * @param[in]  gen_list  Generators variants list terminated with `{NULL, ...}`.
 * @param[in]  intf      Pointer to the interface.
 * @param[in]  param     ASCIIZ string with `value` from the `--param=value` argument.
 * @param[out] gi        Output buffer for writing pointers to the generator name,
 *                       default create/free and get_bits/get_sum callbacks.
 * @return 0 - failure, 1 - success.
 */
static inline int
GeneratorParamVariant_find(const GeneratorParamVariant *gen_list,
    const CallerAPI *intf, const char *param, GeneratorInfo *gi)
{
    gi->name     = "Unknown";
    gi->nbits    = 32;
    gi->create   = default_create;
    gi->free     = default_free;
    gi->get_bits = NULL;
    gi->get_sum  = NULL;
    for (const GeneratorParamVariant *e = gen_list; e->param != NULL; e++) {
        if (!intf->strcmp(param, e->param)) {
            gi->name     = e->name;
            gi->nbits    = e->nbits;
            gi->create   = e->create;
            gi->get_bits = e->get_bits;
            gi->get_sum  = e->get_sum;
            return 1; // Success
        }
    }
    intf->printf("Unknown param value '%s'\n", param);
    return 0; // Failure
}

#define GENERATOR_PARAM_VARIANT_EMPTY {NULL, NULL, 0, NULL, NULL, NULL}


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

#endif // __SMOKERAND_CINTERFACE_H
