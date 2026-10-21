/**
 * @file mwc64k3a3.c
 * @brief mwc64k3a3 is a modification of MWC (multiply-with-carry) generator
 * with three taps and large period.
 * @details Designed for 64-bit computers, resembles Mother-Of-All PRNG from
 * DIEHARD CD-ROM. Developed by P. L'Ecuyer. Passes at least 16 TiB in
 * PractRand 0.96.
 *
 * It uses the next recurrent formula:
 *
 * \f[
 * p = a_1 x_{n-1} + a_2 x_{n-2} + a_3 x_{n-3}) + c_{n-1};~
 * x_n = p \mod b;~
 * c_n = \left\lfloor \frac{p}{b} \right\rfloor
 * \f]
 *
 * `a1 = 0x979f3660cbaca4`, `a2 = 0x31dcb74b96510a8`, `a3 = 0x5194d4649dcd`,
 * `b=2**64`.
 *
 * The generator is equivaled to multiplicative congruential generator with
 * prime modulus and \f$ b^{-1} \mod m \f$ multiplier. The modulus value is
 * \f$ a_3 b^3 + a_2 b^2 + a_1 b - 1\f$. The period is
 * \f$(m - 1) / 2 = 2^{237.35}\f$.
 *
 * \f[
 * m - 1 = 2 \times 3 \times 887 \times 3953923 \times 3623689 \times
 *         2355281837 \times 3135112198551084309513051045954941486630353837
 * \f]
 *
 * References:
 *
 * 1. https://github.com/pierrelecuyer/LatMRG/blob/master/examples/TestMWCSpeed.cc
 * 2. https://web.archive.org/web/20160125103112/http://stat.fsu.edu/pub/diehard/
 *
 * @copyright mwc64k3a3 generator was suggested by P. L'Ecuyer.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x[3];
    uint64_t c;
} Mwc64k3a3State;

static inline uint64_t get_bits_raw(Mwc64k3a3State *obj)
{
    const uint64_t x0 = obj->x[0];
    uint64_t sum_hi, sum_lo, prod_hi, prod_lo;
    sum_lo  = unsigned_mul128(0x979f3660cbaca4,  obj->x[0], &sum_hi);  // x(n-1)
    unsigned_add128(&sum_hi, &sum_lo, obj->c);                         // carry
    prod_lo = unsigned_mul128(0x31dcb74b96510a8, obj->x[1], &prod_hi); // x(n-2)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    prod_lo = unsigned_mul128(0x5194d4649dcd,    obj->x[2], &prod_hi); // x(n-3)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    obj->x[2] = obj->x[1];
    obj->x[1] = obj->x[0];
    obj->x[0] = sum_lo;
    obj->c = sum_hi;
    return x0;
}

static void *create(const CallerAPI *intf)
{
    Mwc64k3a3State *obj = intf->malloc(sizeof(Mwc64k3a3State));
    obj->x[0] = intf->get_seed64();
    obj->x[1] = intf->get_seed64();
    obj->x[2] = intf->get_seed64();
    obj->c = 0x12345678;
    return obj;
}


/**
 * @brief An internal self-test with values obtained from the Python 3.x
 * script (see the `..\misc\rwc.py` file)
 */
static int run_self_test(const CallerAPI *intf)
{
    Mwc64k3a3State obj = {.x = {77777, 56789, 12345}, .c = 12345};
    uint64_t u, u_ref = 0x6797BB8FBE78A18F;
    for (size_t i = 0; i < 100002; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Mwc64k3a3", run_self_test)
