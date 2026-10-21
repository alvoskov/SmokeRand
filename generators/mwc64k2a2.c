/**
 * @file mwc64k2a2.c
 * @brief mwc64k2a2 is a modification of MWC (multiply-with-carry) generator
 * with two taps and large period.
 * @details Designed for 64-bit computers, resembles Mother-Of-All PRNG from
 * DIEHARD CD-ROM. Developed by P. L'Ecuyer. Passes at least 16 TiB in
 * PractRand 0.96.
 *
 * It uses the next recurrent formula:
 *
 * \f[
 * p = a_1 x_{n-1} + a_2 x_{n-2} + c_{n-1};~
 * x_n = p \mod b;~
 * c_n = \left\lfloor \frac{p}{b} \right\rfloor
 * \f]
 * `a1 = 0x2ae390b92740f6d`, `a2 = 0x6fcce264fcc37`, `b = 2**64`.
 *
 * The generator is equivaled to multiplicative congruential generator with
 * prime modulus and \f$ b^{-1} \mod m \f$ multiplier. The modulus value is
 * \f$ a_2 b^2 + a_1 b - 1\f$. The period is \f$(m - 1) / 2 = 2^{177.80}\f$.
 *
 * \f[
 * m - 1 = 2 \times 79 \times 1621 \times 11411 \times 229001607311628305290681164517733587836296671
 * \f]
 *
 * References:
 *
 * 1. https://github.com/pierrelecuyer/LatMRG/blob/master/examples/TestMWCSpeed.cc
 * 2. https://web.archive.org/web/20160125103112/http://stat.fsu.edu/pub/diehard/
 *
 * @copyright mwc64k2a2 generator was suggested by P. L'Ecuyer.
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
    uint64_t x0; ///< x_{-1}
    uint64_t x1; ///< x_{-2}
    uint64_t c;  ///< carry
} Mwc64k2a2State;


static inline uint64_t get_bits_raw(Mwc64k2a2State *obj)
{
    const uint64_t out = obj->x0;
    uint64_t sum_hi, sum_lo, prod_hi, prod_lo;
    sum_lo  = unsigned_muladd128(0x2ae390b92740f6d, obj->x0, obj->c, &sum_hi); // ax(n-1) + c
    prod_lo = unsigned_mul128(0x6fcce264fcc37, obj->x1, &prod_hi);   // x(n-2)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    obj->x1 = obj->x0;
    obj->x0 = sum_lo;
    obj->c = sum_hi;
    return out;
}


static void *create(const CallerAPI *intf)
{
    Mwc64k2a2State *obj = intf->malloc(sizeof(Mwc64k2a2State));
    obj->x0 = intf->get_seed64();
    obj->x1 = intf->get_seed64();
    obj->c = 0x12345678;
    return obj;
}


/**
 * @brief An internal self-test with values obtained from the Python 3.x
 * script (see the `..\misc\rwc.py` file)
 */
static int run_self_test(const CallerAPI *intf)
{
    Mwc64k2a2State obj = {.x0 = 77777, .x1 = 56789, .c = 12345};
    uint64_t u, u_ref = 0xADF0AA7CD59A2D1C;
    for (size_t i = 0; i < 100002; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Mwc64k2a2", run_self_test)
