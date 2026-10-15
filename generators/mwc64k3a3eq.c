/**
 * @file mwc64k3a3eq.c
 * @brief mwc64k3a3eq is a modification of MWC (multiply-with-carry) generator
 * with three taps and large period.
 * @details Designed for 64-bit computers, resembles Mother-Of-All PRNG from
 * DIEHARD CD-ROM. Developed by P. L'Ecuyer.
 *
 * It uses the next recurrent formula (\f$ a=\mathrm{1D495210185C}_{16}\f$):
 *
 * \f[
 * p = a \cdot (x_{n-1} + x_{n-2} + x_{n-3}) + c
 * \f]
 *
 * \f[
 * \f]
 *
 * The generator is equivaled to multiplicative congruential generator with
 * prime modulus and \f$ b^{-1} \mod m \f$ multiplier. The modulus value is
 * \f$ a(b^3 + b^2 + b^1) - 1\f$. The period is \f$(m - 1) / 2 = 2^{235.87}\f$.
 *
 * \f[
 * m - 1 = 2 \times 59 \times 280870353198976367 \times 6113234373790461327037 \times 997622314422762183552453202943
 * \f]
 *
 * References:
 *
 * 1. https://github.com/pierrelecuyer/LatMRG/blob/master/examples/TestMWCSpeed.cc
 * 2. https://web.archive.org/web/20160125103112/http://stat.fsu.edu/pub/diehard/
 *
 * @copyright mwc64k3a3eq generator was suggested by P. L'Ecuyer.
 *
 * Reentrant implementat
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * KISSS64 algorithm was developed by George Marsaglia.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x[3];
    uint64_t c;
} Mwc64k3a3eqState;

// prod = 
static inline uint64_t get_bits_raw(Mwc64k3a3eqState *obj)
{
    const uint64_t a = 0x1d495210185c, x0 = obj->x[0];
    uint64_t sum_hi = 0, sum_lo = obj->x[0], tmp;
    unsigned_add128(&sum_hi, &sum_lo, obj->x[1]);
    unsigned_add128(&sum_hi, &sum_lo, obj->x[2]);
    sum_lo = unsigned_muladd128(a, sum_lo, obj->c, &tmp);
    sum_hi = a * sum_hi + tmp;
    obj->x[2] = obj->x[1];
    obj->x[1] = obj->x[0];
    obj->x[0] = sum_lo;
    obj->c = sum_hi;
    return x0;
}

static void *create(const CallerAPI *intf)
{
    Mwc64k3a3eqState *obj = intf->malloc(sizeof(Mwc64k3a3eqState));
    obj->x[0] = intf->get_seed64();
    obj->x[1] = intf->get_seed64();
    obj->x[2] = intf->get_seed64();
    obj->c = 0x12345678;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Mwc64k3a3eqState obj = {.x = {77777, 56789, 12345}, .c = 12345};
    uint64_t u, u_ref = 0x5E2F3B2EA632A7E2;
    for (size_t i = 0; i < 100002; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Mwc64k3a3eq", run_self_test)
