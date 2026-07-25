/**
 * @file tf0duper32.c
 * @brief Tf0Duper32 is a high-quality combined PRNG inspired by SuperDuper
 * generators family developed by G.Marsaglia.
 * @details Differences from SuperDuper:
 *
 * 1. Klimov-Shamir "crazy" T-function TF0 instead of LCG.
 * 2. xorrot32 (custom LFSR developed by A.L.Voskov) instead of xorshift32.
 * 3. More advanced output function that hides artefacts in lower bits.
 *
 * Its period is \f$ (2^{32} - 1) 2^{32} \f$.
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Tf0Duper32 PRNG state
 */
typedef struct {
    uint32_t tf; ///< TF0 part
    uint32_t xs; ///< xorrot64 (LFSR) part
} Tf0Duper32State;


static inline uint64_t get_bits_raw(Tf0Duper32State *obj)
{
    const uint32_t s = rotl32(obj->tf, 5) + obj->xs;
    obj->tf += obj->tf * obj->tf | 0x4005;
    obj->xs ^= obj->xs << 1;
    obj->xs ^= rotl32(obj->xs, 9) ^ rotl32(obj->xs, 27);
    return s ^ (s >> 16); // To prevent PractRand failure at 8 TiB
}


static void *create(const CallerAPI *intf)
{
    Tf0Duper32State *obj = intf->malloc(sizeof(Tf0Duper32State));
    const uint64_t seed = intf->get_seed64();
    obj->tf = (uint32_t) (seed & 0xFFFFFFFF);
    obj->xs = (uint32_t) (seed >> 32);
    if (obj->xs == 0) {
        obj->xs = 0xDEADBEEF;
    }
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref = 0x657F5782;
    uint32_t x;
    Tf0Duper32State *obj = intf->malloc(sizeof(Tf0Duper32State));
    obj->tf = 123456789; obj->xs = 987654321;
    for (long i = 0; i < 10000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("Observed: 0x%.8lX; expected: 0x%.8lX\n",
        (unsigned long) x, (unsigned long) x_ref);
    intf->free(obj);
    return (x == x_ref) ? 1 : 0;
}

MAKE_UINT32_PRNG("Tf0Duper32", run_self_test)
