/**
 * @file combo.c
 * @brief A combined generator made from a tiny multiplicative lagged Fibonacci
 * generator and MWC generator with base 2^{16}.
 * @details This PRNG suggested by G. Marsaglia and implemented in the DIEHARD
 * test battery. It was manually converted from Fortran to C by Scott Nelson [1].
 * It uses the next recurrent formula:
 *
 * \f[
 * x_{n} = x_{n - 1} \cdot x_{n - 2} \mod 2^{32}
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
 * The period of the generator exceeds \f$ 2^{60} \f$ but it fails a lot of
 * tests in SmokeRand `brief`, `default` and `full` batteries and must not be
 * used as a general purpose generator.
 * 
 * References:
 *
 * 1. http://www.helsbreth.org/random/rng_combo.html
 * 2. https://www.azillionmonkeys.com/qed/programming.html
 *
 * @copyright The original algorithm was suggested by G. Marsaglia.
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"


PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} ComboState;

static void ComboState_init(ComboState *obj, uint32_t seed)
{
    obj->x = seed * 8u + 3u;
    obj->y = seed * 2u + 1u;
    obj->z = seed | 1u;
}

static inline uint64_t get_bits_raw(ComboState *obj)
{
    const uint32_t v = obj->x * obj->y;
    obj->x = obj->y;
    obj->y = v;
    obj->z = (obj->z & 65535u) * 30903u + (obj->z >> 16);
    return obj->y + obj->z;
}

static void *create(const CallerAPI *intf)
{
    ComboState *obj = intf->malloc(sizeof(ComboState));
    ComboState_init(obj, intf->get_seed32());
    return obj;
}

MAKE_UINT32_PRNG("Combo", NULL)
