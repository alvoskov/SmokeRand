/**
 * @file wich1982.c
 * @brief Wichmann-Hill generator (1982 version).
 * @details This implementation is based on integer arithmetics and returns
 * 32-bit unsigned integers instead of single-precision floats. It fails
 * the `brief`, `default` and `full` SmokeRand batteries. It also fails
 * PractRand 0.94 at 512 GiB.
 *
 * `default` battery results: even higher bits are rather low quality.
 * 
 *        # Test name                    xemp              p Interpretation 
 *    ----------------------------------------------------------------------
 *        4 bspace64_1d           3.54233e+07              0 FAIL           
 *        7 bspace32_2d           3.53889e+06              0 FAIL           
 *        8 bspace32_2d_high      3.54023e+06              0 FAIL           
 *        9 bspace21_3d                679269              0 FAIL           
 *       10 bspace21_3d_high      2.25244e+06              0 FAIL           
 *       11 bspace16_4d                532703              0 FAIL           
 *       12 bspace16_4d_high      2.74057e+06              0 FAIL           
 *       13 bspace8_8d                   9253              0 FAIL           
 *       14 bspace8_8d_high            242370              0 FAIL           
 *       17 bspace4_16d_high             2116              0 FAIL           
 *       18 collover20_2d                5407   1 - 1.08e-04 SUSPICIOUS     
 *       19 collover20_2d_high           5389   1 - 4.04e-05 SUSPICIOUS     
 *       38 linearcomp_low              45441              0 FAIL           
 *    ----------------------------------------------------------------------
 *
 * These failure are in a good agreement with the failures of TestU01 Crush
 * battery reported by McCullough and Wilson [3]. 
 *
 * References:
 *
 * 1. B.A. Wichmann, I.D. Hill. Algorithm AS 183, An Efficient and Portable
 *    Pseudo-Random Number Generator // Journal of the Royal Statistical
 *    Society Series C: Applied Statistics. 1982. V. 31. N 2. P. 188-190
 *    https://doi.org/10.2307/2347988
 * 2. B.A. Wichmann, I.D. Hill. Correction: Algorithm AS 183: An Efficient
 *    and Portable Pseudo-Random Number Generator // Journal of the Royal
 *    Statistical Society. Series C (Applied Statistics). 1984. V. 33. N 1.
 *    P. 123. https://doi.org/10.2307/2347676
 * 3. B.D. McCullough, Berry Wilson. On the accuracy of statistical procedures
 *    in Microsoft Excel 2003 // Computational Statistics & Data Analysis.
 *    2005. V. 49. N 4. P. 1244-1252.
 *    https://doi.org/10.1016/j.csda.2004.06.016.
 *
 * @copyright The algorithm was suggested by B.A. Wichmann and I.D. Hill.
 * 
 * Implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t s1;
    uint16_t s2;
    uint16_t s3;
} Wich1982State;

static uint64_t get_bits_raw(Wich1982State *obj)
{
    // Update generator state
    uint64_t s1 = obj->s1, s2 = obj->s2, s3 = obj->s3;
    s1 = (171U * s1) % 30269U;
    s2 = (172U * s2) % 30307U;
    s3 = (170U * s3) % 30323U;
    obj->s1 = (uint16_t) s1;
    obj->s2 = (uint16_t) s2;
    obj->s3 = (uint16_t) s3;
    // Output function
    s1 = (s1 << 32) / 30269U;
    s2 = (s2 << 32) / 30307U;
    s3 = (s3 << 32) / 30323U;
    return (s1 + s2 + s3) & 0xFFFFFFFF;
}

static void *create(const CallerAPI *intf)
{
    Wich1982State *obj = intf->malloc(sizeof(Wich1982State));
    uint64_t s = intf->get_seed64();
    obj->s1 = (uint16_t) (1 + (s % 30000U));
    obj->s2 = (uint16_t) (1 + ((s >> 16) % 30000U));
    obj->s3 = (uint16_t) (1 + ((s >> 32) % 30000U));
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[8] = {
        0xB3685156, 0x0AE498E0, 0xC1A4F757, 0x03DB3127,
        0x88AAA66D, 0xE8978276, 0xBC1C8373, 0x05109F87
    };
    Wich1982State obj = {1, 2, 3};
    int is_ok = 1;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(&obj);
    }
    for (size_t i = 0; i < 8; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(&obj);
        intf->printf("Out = %8.8lX; ref = %8.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT32_PRNG("Wich1982", run_self_test)
