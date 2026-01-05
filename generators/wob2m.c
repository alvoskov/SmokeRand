/**
 * @file wob2m.c
 * @brief WOB2M ("Wrangler Of Bits, 2 mixing variables, with Multiply")
 * is a nonlinear PNG suggested by Bob Jenkins (aka Robert J. Jenkins Jr.) 
 * @details This nonlinear generator has a period not shorter than 2^64
 * and 192 bits of state. It passes `express`, `brief`, `default`, `full`
 * and `birthday` batteries of SmokeRand.
 *
 * Reference: https://burtleburtle.net/bob/rand/wob.html
 * @copyright The WOB2M algorithms is developed by Bob Jenkins (aka
 * Robert J. Jenkins Jr.)
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief WOB2M PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t count;
} Wob2MState;


static inline uint64_t get_bits_raw(Wob2MState *obj)
{
    uint64_t temp = obj->a + obj->count++;
    obj->a = obj->b + rotl64(temp, 12);
    obj->b = 0x0581af43eb71d8b3 * temp ^ rotl64(obj->a, 28);
    return obj->b;
}

/**
 * @brief Initialized a generator: loads seeds to its nonlinear part and
 * makes 10 steps for its warm-up.
 * @param obj The generator to be initialized
 * @param s0  Seed 0, arbitrary.
 * @param s1  Seed 1, arbitrary.
 */
static void Wob2MState_init(Wob2MState *obj, uint64_t s0, uint64_t s1)
{
    obj->a = s0;
    obj->b = s1;
    obj->count = (uint64_t) -10;
    for (int i = 0; i < 10; i++) {
        get_bits_raw(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Wob2MState *obj = intf->malloc(sizeof(Wob2MState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    Wob2MState_init(obj, s0, s1);
    return obj;
}

/**
 * @brief An internal self-test that uses a value obtained from the original
 * implementation by Bob Jenkins.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref = 0x89C6ACCDCAC3F1B0;
    uint64_t u;
    Wob2MState obj;
    Wob2MState_init(&obj, 0, 0);
    for (int i = 0; i < 10000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: %llX; ref: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;

}

MAKE_UINT64_PRNG("WOB2M", run_self_test)
