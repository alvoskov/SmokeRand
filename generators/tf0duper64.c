/**
 * @file tf0duper64.c
 * @brief Tf0Duper64 is a high-quality combined PRNG inspired by SuperDuper
 * generators family developed by G.Marsaglia.
 * @details Differences from SuperDuper:
 *
 * 1. Klimov-Shamir "crazy" T-function TF0 instead of LCG.
 * 2. xorrot64 (custom LFSR developed by A.L.Voskov) instead of xorshift64.
 * 3. More advanced output function that hides artefacts in lower bits.
 *
 * Its period is \f$ (2^{64} - 1) 2^{64} \f$.
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
 * @brief Tf0Duper64 PRNG state
 */
typedef struct {
    uint64_t tf; ///< TF0 part
    uint64_t xs; ///< xorrot64 (LFSR) part
} Tf0Duper64State;


static inline uint64_t get_bits_raw(Tf0Duper64State *obj)
{
    const uint64_t s = rotl64(obj->tf, 11) + obj->xs;
    obj->tf += obj->tf * obj->tf | 0x40000005;
    obj->xs ^= obj->xs << 5;
    obj->xs ^= rotl64(obj->xs, 13) ^ rotl64(obj->xs, 47);
    return s ^ (s >> 32);
}


static void *create(const CallerAPI *intf)
{
    Tf0Duper64State *obj = intf->malloc(sizeof(Tf0Duper64State));
    obj->tf = intf->get_seed64();
    obj->xs = intf->get_seed64();
    if (obj->xs == 0) {
        obj->xs = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Tf0Duper64", NULL)
