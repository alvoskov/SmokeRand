/**
 * @file smwc48.c
 * @brief MWC128 - 128-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^127.
 * Passes SmallCrush, Crush and BigCrush tests.
 * The MWC_A1 multiplier was found by S. Vigna.
 *
 * References:
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC128 state. Cannot be initialized to (0, 0) or to
 * (2^64 - 1, 2^64 - 1). Default initialization is (seed, 1) as suggested
 * by S. Vigna.
 */
typedef struct {
    uint16_t x;
    uint16_t x2;
    uint16_t c;
} Smwc48State;


/**
 * @brief MWC128 PRNG implementation.
 * @details Optimization of the output function (made for the very small bad
 * multiplier 0x621):
 *
 * 1. Optimize the shift in the x_new ^ (x_2 <<< sh1) output.
 * 2. Test if for the (Q x_new) ^ (x_2 <<< sh1) output.
 */
static inline uint16_t get_bits16(Smwc48State *obj)
{
    static const uint32_t MWC_A1 = 0xfefa;//0xe8c5;
    static const uint16_t LCG_A1 = 62317u;
    uint16_t out = (uint16_t) ((LCG_A1 * obj->x) ^ ((obj->x2 << 5) | (obj->x2 >> 11)));
//    uint16_t out = obj->x ^ obj->x2;
    uint32_t mul = MWC_A1 * obj->x + obj->c;
    obj->c = (uint16_t) (mul >> 16);
    obj->x2 = obj->x;
    obj->x = (uint16_t) (mul & 0xFFFF);
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = get_bits16(state);
    uint32_t lo = get_bits16(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Smwc48State *obj = intf->malloc(sizeof(Smwc48State));
    obj->x  = (uint16_t) intf->get_seed32();
    obj->x2 = (uint16_t) intf->get_seed32();
    obj->c = 1;
    return obj;
}


MAKE_UINT32_PRNG("SMWC48", NULL)
