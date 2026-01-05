/**
 * @file ultra.c
 * @brief A combined generator made from a tiny multiplicative lagged Fibonacci
 * generator and MWC generator with base 2^{16}.
 * @details This PRNG suggested by G. Marsaglia and implemented in DIEHARD
 * test suite. Its version with reduced state was suggested by Scott Nelson [1].
 * It uses the next recurrent formula:
 *
 * \f[
 * x_{n} = x_{n - 17} \cdot x_{n - 5} \mod 2^{32}
 * \f]
 *
 * \f[
 * y_{n} = 30903*y_{n - 1} + c_{n-1} \mod 2^{16}
 * \f]
 *
 * \f[
 * u_{n} = x_{n} + y_{n} \mod 2^{32}
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
    uint32_t x[ULTRA_R]; ///< Lagged Fibonacci generator state.
    uint32_t mwc; ///< MWC generator state.
    int r; ///< Lagged Fibonacci generator state: pointer 1.
    int s; ///< Lagged Fibonacci generator state: pointer 2.
} UltraState;


static void UltraState_init(UltraState *obj, uint64_t seed)
{
    obj->mwc = 15;
    for (int i = 0; i < ULTRA_R; i++) {
        obj->x[i] = (uint32_t) ( pcg_bits64(&seed) << 2 ) | 0x1;
    }
    obj->r = ULTRA_R - 1;
    obj->s = ULTRA_S - 1;
}


static uint64_t get_bits_raw(UltraState *obj)
{
    const uint32_t u = obj->x[obj->r] * obj->x[obj->s];
    obj->x[obj->r] = u;
    if (obj->r == 0) {
        obj->r = ULTRA_R;
    }
    --obj->r;
    if (obj->s == 0) {
        obj->s = ULTRA_R;
    }
    --obj->s;
    obj->mwc = (obj->mwc & 65535u) * 30903u + (obj->mwc >> 16);
    return u + obj->mwc;
}

static void *create(const CallerAPI *intf)
{
    UltraState *obj = intf->malloc(sizeof(UltraState));
    UltraState_init(obj, intf->get_seed64());
    return obj;
}


MAKE_UINT32_PRNG("ultra", NULL)
