/**
 * @file mwc64.c
 * @brief MWC64 - 64-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^63.
 *
 * @details Multiply-with-carry PRNG that just returns x.
 * Has a period about 2^63, passes SmallCrush but fails Crush and BigCrush
 * batteries from TestU01.
 *
 * This type of PRNG was proposed by G. Marsaglia. The A0 multiplier was
 * found by A.L. Voskov using spectral test from TAOCP and SmallCrush
 * for the version without XORing.
 *
 * This PRNG fails "birthdayspacing t=3 (N12)" test from Crush battery.
 * The similar problem was noticed by S.Vigna in MWC128 on much larger
 * samples.
 *
 * References:
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. https://github.com/lpareja99/spectral-test-knuth
 * 3. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
 *
 * SmokeRand test results:
 *
 *        # Test name            xemp              p Interpretation  Thr#
 *    -----------------------------------------------------------------------
 *        4 bspace64_1d           897   1 - 4.94e-04 SUSPICIOUS         6
 *        7 bspace32_2d           816   1 - 1.05e-09 SUSPICIOUS         6
 *        8 bspace32_2d_high      861   1 - 3.70e-06 SUSPICIOUS         5
 *        9 bspace21_3d        172306              0 FAIL               6
 *       10 bspace21_3d_high   172791              0 FAIL               5
 *    -----------------------------------------------------------------------
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * The MWC algorithm was proposed by G.Marsaglia.
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC64 state.
 */
typedef struct {
    uint64_t data;
} MWC64State;

static inline uint64_t get_bits_raw(MWC64State *obj)
{
    const uint64_t A0 = 0xff676488; // 2^32 - 10001272
    uint32_t c = (uint32_t) (obj->data >> 32);
    uint32_t x = (uint32_t) (obj->data & 0xFFFFFFFF);
    obj->data = A0 * x + c;
    return x;
}

static void *create(const CallerAPI *intf)
{
    MWC64State *obj = intf->malloc(sizeof(MWC64State));
    // Seeding: prevent (0,0) and (?,0xFFFF)
    do {
        obj->data = intf->get_seed64() << 1;
    } while (obj->data == 0);
    return obj;
}

MAKE_UINT32_PRNG("MWC64", NULL)
