/**
 * @file ultra64.c
 * @brief A combined generator made from a tiny multiplicative lagged Fibonacci
 * generator and MWC generator with base 2^{32}.
 * @details The 32-bit version of this PRNG suggested by G. Marsaglia and implemented
 * in DIEHARD test suite. Its version with reduced state but 64-bit variables was
 * suggested by A.L. Voskov.
 *
 * \f[
 * x_{n} = x_{n - 17} \cdot x_{n - 5} \mod 2^{64}
 * \f]
 *
 * \f[
 * y_{n} = a*y_{n - 1} + c_{n-1} \mod 2^{64}
 * \f]
 *
 * \f[
 * u_{n} = x_{n} + y_{n} \mod 2^{64}
 * \f]
 *
 * References:
 *
 * 1. http://www.helsbreth.org/random/rng_combo.html
 * 2. https://www.azillionmonkeys.com/qed/programming.html
 *
 * @copyright The original algorithm was suggested by G. Marsaglia.
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define ULTRA_R 17
#define ULTRA_S 5

typedef struct {
    uint64_t x[ULTRA_R]; ///< Lagged Fibonacci generator state.
    uint64_t mwc; ///< MWC generator state.
    int r; ///< Lagged Fibonacci generator state: pointer 1.
    int s; ///< Lagged Fibonacci generator state: pointer 2.
} Ultra64State;


static void Ultra64State_init(Ultra64State *obj, uint64_t seed)
{
    obj->mwc = ((~seed) & 0xFFFFFFFF) | (1ull << 33);
    for (int i = 0; i < ULTRA_R; i++) {
        obj->x[i] = ( pcg_bits64(&seed) << 2 ) | 0x1;
    }
    obj->r = ULTRA_R - 1;
    obj->s = ULTRA_S - 1;
}


static uint64_t get_bits_raw(Ultra64State *obj)
{
    static const uint64_t MWC_A = 0xff676488; // 2^32 - 10001272
    const uint64_t u = obj->x[obj->r] * obj->x[obj->s];
    obj->x[obj->r] = u;
    if (obj->r == 0) {
        obj->r = ULTRA_R;
    }
    --obj->r;
    if (obj->s == 0) {
        obj->s = ULTRA_R;
    }
    --obj->s;
    obj->mwc = (obj->mwc & 0xFFFFFFFFu) * MWC_A + (obj->mwc >> 32);
    return u + obj->mwc;
}

static void *create(const CallerAPI *intf)
{
    Ultra64State *obj = intf->malloc(sizeof(Ultra64State));
    Ultra64State_init(obj, intf->get_seed64());
    return obj;
}


MAKE_UINT64_PRNG("ultra64", NULL)
