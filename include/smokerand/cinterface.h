#ifndef __SMOKERAND_CINTERFACE_H
#define __SMOKERAND_CINTERFACE_H
#include "smokerand/core.h"

#define PRNG_CMODULE_PROLOG SHARED_ENTRYPOINT_CODE

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

/**
 * @brief  Some default boilerplate code for scalar PRNG that returns
 * unsigned 32-bit numbers.
 * @details  Requires the next functions to be defined:
 *
 * - `static uint64_t get_bits(void *state);`
 * - `static void *create(CallerAPI *intf);`
 *
 * It also relies on default prolog (intf static variable, some exports etc.),
 * see PRNG_CMODULE_PROLOG
 */
#define MAKE_UINT32_PRNG(prng_name, selftest_func) \
EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); } \
GET_SUM_FUNC \
int EXPORT gen_getinfo(GeneratorInfo *gi) { \
    gi->name = prng_name; \
    gi->create = create; \
    gi->get_bits = get_bits; \
    gi->get_sum = get_sum; \
    gi->nbits = 32; \
    gi->self_test = selftest_func; \
    return 1; \
}

/**
 * @brief  Some default boilerplate code for scalar PRNG that returns
 * unsigned 32-bit numbers.
 * @details  Requires the next functions to be defined:
 *
 * - `static uint64_t get_bits(void *state);`
 * - `static void *create(CallerAPI *intf);`
 *
 * It also relies on default prolog (intf static variable, some exports etc.),
 * see PRNG_CMODULE_PROLOG
 */
#define MAKE_UINT64_PRNG(prng_name, selftest_func) \
EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); } \
GET_SUM_FUNC \
int EXPORT gen_getinfo(GeneratorInfo *gi) { \
    gi->name = prng_name; \
    gi->create = create; \
    gi->get_bits = get_bits; \
    gi->get_sum = get_sum; \
    gi->nbits = 64; \
    gi->self_test = selftest_func; \
    return 1; \
}

#endif
