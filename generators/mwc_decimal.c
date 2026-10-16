/**
 * @file mwc_decimal.c
 * @brief Decimal combined multiply-with-carry generator initially
 * designed for pocket calculators.
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
    const uint64_t p0 = 97377U * (uint64_t)obj->x0 + obj->c0;
    const uint64_t p1 = 97587U * (uint64_t)obj->x1 + obj->c1;
    obj->c0 = (uint32_t) (p0 / 100000U);
    obj->x0 = (uint32_t) (p0 % 100000U);
    obj->c1 = (uint32_t) (p1 / 100000U);
    obj->x1 = (uint32_t) (p1 % 100000U);
    const uint32_t out = (obj->x0 + obj->x1) % 100000U;
    return out;//decrot5(out, obj->c1 % 10000U);
}


static inline uint64_t get_bits_raw(MwcDecim *obj)
{
    uint64_t x_hi = get_decim(obj);
    uint64_t x_lo = get_decim(obj);
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
