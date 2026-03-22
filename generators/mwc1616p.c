/**
 * @file mwc1616p.c
 * @brief A modified version of MWC1616 generator suggested by G.Marsaglia.
 * It has period about 2^{62} and designed for 16-bit CPUs.
 *
 * @details MWC1616+ passes BigCrush from TestU01 and all four batteries
 * from SmokeRand. It has much higher quality than the original MWC1616 due
 * to the next modifications:
 *
 * 1. Improved output function that hides problems with 3-dimensional birthday
 *    spacings tests in MWC generators.
 * 2. New multipliers that are much closer to 2^{16} than in the original
 *    MWC1616. Improves equidistribution of bits and allows to pass BCFN
 *    test from PractRand (based on Hamming weights).
 *
 * Note: occasionally shows suspicious values in the test 78 from BigCrush
 * (`RandomWalk1 J (L=10000, r=0)`)
 *
 * References:
 *
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. https://groups.google.com/g/sci.stat.math/c/1kNyF8ixyqc/m/RHeuadKl0ocJ
 * 3. https://github.com/lpareja99/spectral-test-knuth
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC1616+ state.
 */
typedef struct {
    uint32_t z;
    uint32_t w;
} Mwc1616PlusShared;


static inline uint64_t get_bits_raw(Mwc1616PlusShared *obj)
{
    const uint16_t z_lo = (uint16_t) (obj->z & 0xFFFF);
    const uint16_t z_hi = (uint16_t) (obj->z >> 16);
    const uint16_t w_lo = (uint16_t) (obj->w & 0xFFFF);
    const uint16_t w_hi = (uint16_t) (obj->w >> 16);
    obj->z = (uint32_t)61578U * z_lo + z_hi;
    obj->w = (uint32_t)63885U * w_lo + w_hi;
    const uint32_t mwc = rotl32(obj->z, 16) + obj->w;
    return mwc;
}


static void *create(const CallerAPI *intf)
{
    Mwc1616PlusShared *obj = intf->malloc(sizeof(Mwc1616PlusShared));
    const uint32_t seed0 = intf->get_seed32();
    obj->z = (seed0 & 0xFFFF) | ((uint32_t)1U << 16U);
    obj->w = (seed0 >> 16)    | ((uint32_t)1U << 16U);
    return obj;
}


MAKE_UINT32_PRNG("Mwc1616+", NULL)
