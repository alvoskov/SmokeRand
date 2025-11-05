/**
 * @file ncombo.c
 * @brief A combined generator made from a tiny multiplicative lagged Fibonacci
 * generator and MWC generator with base 2^{16}.
 * @details This PRNG suggested by G. Marsaglia and posted by Scott Nelson [1].
 * It uses the next recurrent formula:
 *
 * \f[
 * x_{n} = x_{n - 1} \cdot x_{n - 2} \mod 2^{32}
 * \f]
 *
 * \f[
 * y_{n} = y_{n - 3} - y_{n - 1} \mod 2^{32} - 5
 * \f]
 *
 * \f[
 * u_{n} = x_{n} - y_{n} \mod 2^{32}
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
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"


PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t x[2];
    uint32_t y[3];
} NcomboState;

static void NcomboState_init(NcomboState *obj, uint64_t seed)
{
    obj->x[0] = (uint32_t) ((seed & 0xFFFFFFFF) * 8u + 3u);
    obj->x[1] = (uint32_t) ((seed & 0xFFFFFFFF) * 2u + 1u);

    obj->y[0] = (uint32_t) (seed >> 32);
    obj->y[1] = 0xCAFEBABE;
    obj->y[2] = 0xDEADBEEF;
}

static inline uint64_t get_bits_raw(void *state)
{
    NcomboState *obj = state;
    // Multiplicative part
    uint32_t v_mul = obj->x[0] * obj->x[1];
    obj->x[0] = obj->x[1];
    obj->x[1] = v_mul;
    // Subtractive part
    uint32_t v_sub = obj->y[0] - obj->y[2];
    if (obj->y[0] < obj->y[2]) {
        v_sub -= 5;
    }
    obj->y[0] = obj->y[1];
    obj->y[1] = obj->y[2];
    obj->y[2] = v_sub;
    // Combined output
    return v_mul - v_sub;
}

static void *create(const CallerAPI *intf)
{
    NcomboState *obj = intf->malloc(sizeof(NcomboState));
    NcomboState_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT32_PRNG("Ncombo", NULL)
