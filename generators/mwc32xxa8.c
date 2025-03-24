/**
 * @file mwc128xxa32.c
 * @brief MWC128XXA32.
 * @details Multiply-with-carry PRNG.
 *
 * \f[
 * x_{n} = ax_{n - 3} + c \mod 2^32
 * \f]
 * 
 *
 * References:
 * 1. https://tom-kaitchuck.medium.com/designing-a-new-prng-1c4ffd27124d
 * 2. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 3. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC32XXA8 state. Cannot be initialized to (0, 0, 0, 0) or to
 * (2^64 - 1, 2^32 - 1, 2^32 - 1, 2^64 - 1). Default initialization
 * is (seed, seed, seed, 1) as suggested by S. Vigna.
 */
typedef struct {
    uint8_t x[3];
    uint8_t c;
} Mwc32xxa8State;

static inline uint64_t get_bits_raw(void *state)
{
    static const uint8_t MWC_A1 = 228;
    Mwc32xxa8State *obj = state;
    uint32_t ans = 0;
    for (int i = 0; i < 4; i++) {
        uint16_t t = MWC_A1 * (uint64_t) obj->x[2];
        uint8_t ans8 = (obj->x[2] ^ obj->x[1]) + (obj->x[0] ^ (t >> 8));
        t += obj->c;
        obj->x[2] = obj->x[1];
        obj->x[1] = obj->x[0];
        obj->x[0] = t;
        obj->c = t >> 8;
        ans = (ans << 8) | ans8;
    }
    return ans;
}

static void Mwc32xxa8State_init(Mwc32xxa8State *obj, uint32_t seed)
{
    obj->x[0] = seed & 0xFF;
    obj->x[1] = (seed >> 8) & 0xFF;
    obj->x[2] = (seed >> 16) & 0xFF;
    obj->c = ((seed >> 24) & 0x7F) | 0x1;
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}

static void *create(const CallerAPI *intf)
{
    Mwc32xxa8State *obj = intf->malloc(sizeof(Mwc32xxa8State));
    Mwc32xxa8State_init(obj, intf->get_seed32());
    return (void *) obj;
}


MAKE_UINT32_PRNG("Mwc32xxa8", NULL)
