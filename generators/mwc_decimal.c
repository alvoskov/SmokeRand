/**
 * @file mwc_decimal.c
 * @brief Decimal combined multiply-with-carry generator initially
 * designed for pocket calculators.
 *
 * Variant 1 (smaller constants) without decimation:
 *
 * - BigCrush: fails N14 (BirthdaySpacings, t = 3) 7.4e-7
 * - SmokeRand FULL:
 * 18 bspace64_1d_c256               3295       3.33e-05 SUSPICIOUS
 * 19 bspace32_2d_c256               3403       2.04e-09 SUSPICIOUS
 * 20 bspace32_2d_c256_high          3432       8.53e-11 FAIL      
 * 22 bspace21_3d_c256_high          3338       1.05e-06 SUSPICIOUS
 *
 * But passes BigCrush after addition of pseudorandom rotation at
 * output.
 *
 * Variant 2 (larger constants) without decimation:
 * - BigCrush: passes
 *
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
 * @brief Decimal MWC state.
 */
typedef struct {
    uint32_t c0;
    uint32_t x0;
    uint32_t c1;
    uint32_t x1;
} MwcDecim;


static inline uint32_t decrot5(uint32_t x, uint32_t r)
{
    static const uint32_t p10[] = {1, 10, 100, 1000, 10000, 100000};
    r = r % 5;
    return (x * p10[r] + x / p10[5 - r]) % 100000;
}


static inline uint32_t get_decim(MwcDecim *obj)
{
    const uint64_t a0 = 48549U, a1 = 49551U, b = 100000U;
    //const uint64_t a0 = 97377U, a1 = 97587U, b = 100000U;
    const uint64_t p0 = a0 * (uint64_t)obj->x0 + obj->c0;
    const uint64_t p1 = a1 * (uint64_t)obj->x1 + obj->c1;
    obj->c0 = (uint32_t) (p0 / b);
    obj->x0 = (uint32_t) (p0 % b);
    obj->c1 = (uint32_t) (p1 / b);
    obj->x1 = (uint32_t) (p1 % b);
    const uint32_t out = (obj->x0 + obj->x1) % b;
    return decrot5(out, obj->c1 % 10000U);
//    return out;
}


static inline uint64_t get_bits_raw(MwcDecim *obj)
{    
    uint64_t x_hi = get_decim(obj);
    //get_decim(obj);
    uint64_t x_lo = get_decim(obj);
    //get_decim(obj);
    uint32_t out = (uint32_t) (0.4294967296 * (double) (x_hi * 100000U + x_lo));
    return out;
}


static void *create(const CallerAPI *intf)
{
    MwcDecim *obj = intf->malloc(sizeof(MwcDecim));
    uint64_t seed0 = intf->get_seed64(); // For MWC
    obj->c0 = 1;
    obj->x0 = seed0 & 0x1FFF;
    obj->c1 = 1;
    obj->x1 = (seed0 >> 16) & 0x1FFF;
    return obj;
}


MAKE_UINT32_PRNG("MWCDecim", NULL)
