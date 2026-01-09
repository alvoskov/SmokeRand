/**
 * @file mwc40xxa8.c
 * @brief MWC40XXA8.
 * @details Multiply-with-carry PRNG.
 *
 * \f[
 * x_{n} = ax_{n - 4} + c \mod 2^32
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
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC40XXA8 state. Cannot be initialized to (0, 0, 0, 0) or to
 * (2^8 - 1, 2^8 - 1, 2^8 - 1, carry_max). Default initialization
 * is (seed, seed, seed, 1) as suggested by S. Vigna.
 */
typedef struct {
    uint8_t x[4];
    uint8_t c;
} Mwc40xxa8State;


static inline uint64_t get_bits_raw(Mwc40xxa8State *obj)
{
    static const uint16_t MWC_A1 = 227U;
    uint32_t ans = 0;
    for (int i = 0; i < 4; i++) {
        uint16_t t = (uint16_t) (MWC_A1 * (uint16_t) obj->x[3]);
        uint8_t ans8 = (uint8_t) ((obj->x[2] ^ obj->x[1]) + (obj->x[0] ^ (t >> 8)));
        t = (uint16_t) (t + obj->c);
        obj->x[3] = obj->x[2];
        obj->x[2] = obj->x[1];
        obj->x[1] = obj->x[0];
        obj->x[0] = (uint8_t) t;
        obj->c = (uint8_t) (t >> 8);
        ans = (ans << 8) | ans8;
    }
    return ans;
}

static void Mwc40xxa8State_init(Mwc40xxa8State *obj, uint32_t seed)
{
    obj->x[0] = (uint8_t) (seed & 0xFF);
    obj->x[1] = (uint8_t) ((seed >> 8) & 0xFF);
    obj->x[2] = (uint8_t) ((seed >> 16) & 0xFF);
    obj->x[3] = (uint8_t) ((seed >> 24) & 0xFF);
    obj->c = 1;
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}

static void *create(const CallerAPI *intf)
{
    Mwc40xxa8State *obj = intf->malloc(sizeof(Mwc40xxa8State));
    Mwc40xxa8State_init(obj, intf->get_seed32());
    return obj;
}


MAKE_UINT32_PRNG("Mwc40xxa8", NULL)
