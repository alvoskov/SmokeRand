/**
 * @file grube73_mrg2.c
 * @brief A very old MRG (multiple recursive generator) with 31-bit output.
 * @details It uses the next recurrent formula:
 *
 * \f[
 * x_i = a_1 x_{i-1} + a_2 x_{i-2} \mod m
 * \f]
 *
 * where \f$m = 2^{31} - 1\f$ (prime modulus),
 * \f$a_1 = 337190270\f$, \f$a_2 = 268152554\f$.
 * Its period is \f$ m^2 - 1 f\f$ or approximately \f$2^{62}\f$.
 *
 * Crush results give occasional suspicious values, but it is not
 * fully reproducible, in some cases even non-suspicious p-values may be
 * returned.
 *
 *          Test                          p-value
 *    ----------------------------------------------
 *    13  BirthdaySpacings, t = 4         2.8e-5
 *    ----------------------------------------------
 *    All other tests were passed
 *
 * It does pass `full` SmokeRand battery with `uint31` filter but if the
 * `bspace16_4d_high` test is used with larger number of samples it returns
 * suspicious values:
 *
 *     bspace16_4d_high test=bspace_nd nbits_per_dim=16 ndims=4 nsamples=400  get_lower=0 end
 *
 * BigCrush results (3D suspicious values are systematic):
 *
 *          Test                          p-value
 *    ----------------------------------------------
 *    14  BirthdaySpacings, t = 3         1.4e-4
 *    15  BirthdaySpacings, t = 4        1.7e-30
 *    ----------------------------------------------
 *    All other tests were passed
 *
 * It seems more sensitive than SmokeRand because it takes bits from the
 * middle part of the number and uses other values of \f$\lambda\f$.
 *
 * References:
 *
 * 1. Grube A. Mehrfach rekursiv-erzeugte Pseudo-Zufallszahlen // Z. angew.
 *    Math. Mech. 1973. V. 53. P. 223-225. https://doi.org/10.1002/zamm.197305312116
 * 2. P. L'Ecuyer. History of uniform random number generation. 2017 Winter
 *    Simulation Conference (WSC), Las Vegas, NV, USA, 2017, pp. 202-230.
 *    https://doi.org/10.1109/WSC.2017.8247790
 *
 * How to check the coefficients:
 *
 *    import galois
 *    GFp = galois.GF(2**31 - 1)
 *    poly_to_test = galois.Poly([1, -337_190_270, -268_152_554], field=GFp)
 *    is_primitive = poly_to_test.is_primitive()
 *    print(f"Is {poly_to_test} primitive? {is_primitive}")
 *
 * @copyright
 * grube73_mrg was designed by A. Grube.
 *
 * Reentrant C99 implementation for SmokeRand:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[2];
} Grube73Mrg2State;

static inline uint64_t get_bits_raw(Grube73Mrg2State *obj)
{
    static const uint64_t a[2] = {337190270, 268152554};
    uint64_t s_new = a[0]*obj->s[0] + a[1]*obj->s[1];
    const uint32_t q = (uint32_t) (s_new & 0x7FFFFFFF);
    const uint32_t p = (uint32_t) (s_new >> 31);
    s_new = p + q;
    if (s_new >= 0x7FFFFFFF) {
        s_new -= 0x7FFFFFFF;
    }
    obj->s[1] = obj->s[0];
    obj->s[0] = (uint32_t) s_new;
    return s_new << 1;
}


static void *create(const CallerAPI *intf)
{
    Grube73Mrg2State *obj = intf->malloc(sizeof(Grube73Mrg2State));
    seeds_to_array_u32(intf, obj->s, 2);
    for (int i = 0; i < 2; i++) {
        obj->s[i] &= 0x7FFFFFFF;
        if (obj->s[i] == 0 || obj->s[i] == 0x7FFFFFFF) {
            obj->s[i] = 1;
        }
    }
    return obj;
}

/**
 * @brief The test is based on the values obtained from the next Python script:
 * @details The used script:
 *
 *    a, s = [337_190_270, 268_152_554], [1, 2]
 *    for i in range(0, 10_000_000):
 *        s_new = (a[0]*s[0] + a[1]*s[1]) % (2**31 - 1)
 *        s = [s_new, s[0]]
 *    print(s[0] << 1)
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t u_ref = 2902946792;
    Grube73Mrg2State obj = {.s = {1, 2}};
    uint32_t u;
    for (long i = 0; i < 10000000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("u = %lu, u_ref = %lu\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("grube73_mrg2", run_self_test)
