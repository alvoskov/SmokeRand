#ifndef __SMOKERAND_CINTERFACE_H
#define __SMOKERAND_CINTERFACE_H
#include "smokerand/smokerand_core.h"

#define PRNG_CMODULE_PROLOG SHARED_ENTRYPOINT_CODE

/**
 * @brief  Some default boilerplate code for scalar PRNG that returns
 * unsigned 32-bit numbers.
 * @details  Requires the next functions to be defined:
 *
 * - `static uint32_t get_bits32(void *state);`
 * - `static void *create(CallerAPI *intf);`
 *
 * It also relies on default prolog (intf static variable, some exports etc.),
 * see PRNG_CMODULE_PROLOG
 */
#define MAKE_UINT32_PRNG(prng_name, selftest_func) \
int EXPORT gen_getinfo(GeneratorInfo *gi) { \
    gi->name = prng_name; \
    gi->create = create; \
    gi->get_bits32 = get_bits32; \
    gi->get_bits64 = NULL; \
    gi->self_test = selftest_func; \
    return 1; \
}

/**
 * @brief  Some default boilerplate code for scalar PRNG that returns
 * unsigned 32-bit numbers.
 * @details  Requires the next functions to be defined:
 *
 * - `static uint32_t get_bits32(void *state);`
 * - `static void *create(CallerAPI *intf);`
 *
 * It also relies on default prolog (intf static variable, some exports etc.),
 * see PRNG_CMODULE_PROLOG
 */
#define MAKE_UINT64_PRNG(prng_name, selftest_func) \
int EXPORT gen_getinfo(GeneratorInfo *gi) { \
    gi->name = prng_name; \
    gi->create = create; \
    gi->get_bits32 = NULL; \
    gi->get_bits64 = get_bits64; \
    gi->self_test = selftest_func; \
    return 1; \
}

#endif
