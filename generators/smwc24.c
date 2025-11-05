/**
 * @file smwc24.c
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
    uint8_t x;
    uint8_t x2;
    uint8_t c;
} Smwc24State;


/**
 * @brief MWC128 PRNG implementation.
 * @details Optimization of the output function (made for the very small bad
 * multiplier 0x621):
 *
 * 1. Optimize the shift in the x_new ^ (x_2 <<< sh1) output.
 * 2. Test if for the (Q x_new) ^ (x_2 <<< sh1) output.
 */
static inline uint8_t get_bits8(Smwc24State *obj)
{
    static const uint16_t MWC_A1 = 0x2d;
    static const uint8_t LCG_A1 = 137u;
    uint8_t out = (uint8_t) ((LCG_A1 * obj->x) ^ ( (obj->x2 << 5) | (obj->x2 >> 3)) );
    uint16_t mul = (uint16_t) (MWC_A1 * obj->x + obj->c);
    obj->c = (uint8_t) (mul >> 8);
    obj->x2 = obj->x;
    obj->x = (uint8_t) (mul & 0xFF);
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = get_bits8(state);
    }
    return out.u32;    
}


static void *create(const CallerAPI *intf)
{
    Smwc24State *obj = intf->malloc(sizeof(Smwc24State));
    obj->x = (uint8_t) intf->get_seed32();
    obj->x2 = (uint8_t) intf->get_seed32();
    obj->c = 1;
    return (void *) obj;
}


MAKE_UINT32_PRNG("SMWC24", NULL)
