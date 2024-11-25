/**
 * @file mwc3232x_shared.c
 * @brief A modified version of MWC1616 generator suggested by G.Marsaglia.
 * It has period about 2^{126} and designed for 32-bit CPUs. Returns 64-bit
 * unsigned integers.
 *
 * @details MWC3232X passes BigCrush from TestU01 and all four batteries
 * from SmokeRand. It has much higher quality than the original MWC1616 due
 * to the next modifications:
 *
 * 1. Improved output function: `((x1 ^ c2) << 32) | (x2 ^ c1)` that hides
 *    problems with 3-dimensional birthday spacings tests in MWC generators.
 *    This modification is partially inspired by MWC64X.
 * 2. New multipliers that are much closer to 2^{16} than in the original
 *    MWC1616. Improves equidistribution of bits and allows to pass BCFN
 *    test from PractRand (based on Hamming weights).
 *
 * References:
 *
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. https://groups.google.com/g/sci.stat.math/c/1kNyF8ixyqc/m/RHeuadKl0ocJ
 * 3. David B. Thomas. The MWC64X Random Number Generator.
 *    https://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html
 * 4. https://github.com/lpareja99/spectral-test-knuth
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC3232X state.
 */
typedef struct {
    uint32_t z_hi;
    uint32_t z_lo;
    uint32_t w_hi;
    uint32_t w_lo;
} Mwc3232xShared;

static inline uint64_t get_bits_raw(void *state)
{
    Mwc3232xShared *obj = state;
    uint64_t z_prod = 4294441395 * (obj->z_lo) + (obj->z_hi); // 2^32 - 525901
    obj->z_hi = z_prod >> 32; obj->z_lo = z_prod & 0xFFFFFFFF;
    uint64_t w_prod = 4294440669 * (obj->w_lo) + (obj->w_hi);
    obj->w_hi = w_prod >> 32; obj->w_lo = w_prod & 0xFFFFFFFF; // 2^32 - 526627
    uint64_t mwc = ((uint64_t) (obj->z_lo ^ obj->w_hi) << 32) | (obj->w_lo ^ obj->z_hi);
    return mwc;
}


static void *create(const CallerAPI *intf)
{
    Mwc3232xShared *obj = intf->malloc(sizeof(Mwc3232xShared));
    uint64_t seed0 = intf->get_seed64();
    obj->z_hi = 1;
    obj->z_lo = seed0 >> 32;
    obj->w_hi = 1;
    obj->w_lo = seed0;
    return (void *) obj;
}


MAKE_UINT64_PRNG("Mwc3232x", NULL)
