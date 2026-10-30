/**
 * @file tf0mwc32.c
 * @brief Tf0mwc32 is a high-quality combined PRNG inspired by SuperDuper
 * generators family developed by G.Marsaglia.
 * @details Differences from SuperDuper:
 *
 * 1. Klimov-Shamir "crazy" T-function TF0 instead of LCG.
 * 2. mwc instead of xorshift32
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
 * @brief Tf0Mwc64 PRNG state
 */
typedef struct {
    uint64_t tf;  ///< TF0 part
    uint64_t mwc; ///< xorrot64 (LFSR) part
} Tf0Mwc64State;


static inline uint64_t get_bits_raw(Tf0Mwc64State *obj)
{
    const uint64_t out = rotl64(obj->tf, 8) + obj->mwc;
    obj->tf += obj->tf * obj->tf | 0x40000005U;
    obj->mwc = 4027340273U * (obj->mwc & 0xFFFFFFFFU) + (obj->mwc >> 32);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Tf0Mwc64State *obj = intf->malloc(sizeof(Tf0Mwc64State));
    obj->tf = intf->get_seed64();
    obj->mwc = intf->get_seed64() >> 1;
    if (obj->mwc == 0) {
        obj->mwc = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Tf0mwc64", NULL)
