/**
 * @file mwc32xxa8.c
 * @brief MWC128XXA8 geenerator.
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
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC32XXA8 state. Cannot be initialized to (0, 0, 0, 0) or to
 * (255, 255, 255, 227). Default initialization is (seed, seed, seed, 1).
 */
typedef struct {
    uint8_t x[3]; ///< \f$ x_i \f$ buffer.
    uint8_t c;    ///< Carry value.
} Mwc32xxa8State;

static inline uint64_t get_bits_raw(Mwc32xxa8State *obj)
{
    static const uint16_t MWC_A1 = 228U;
    uint32_t ans = 0;
    for (int i = 0; i < 4; i++) {
        uint16_t t = (uint16_t) (MWC_A1 * (uint16_t) obj->x[2]);
        uint8_t ans8 = (uint8_t) ( (obj->x[2] ^ obj->x[1]) + (obj->x[0] ^ (t >> 8)) );
        t = (uint16_t) (t + obj->c);
        obj->x[2] = obj->x[1];
        obj->x[1] = obj->x[0];
        obj->x[0] = (uint8_t) t;
        obj->c = (uint8_t) (t >> 8);
        ans = (ans << 8) | ans8;
    }
    return ans;
}

/**
 * @brief Initialize the generator state.
 * @param obj   Generator state to be initialized.
 * @param seed  Will be used for filling the generator state.
 * @memberof Mwc32xxa8State
 */
static void Mwc32xxa8State_init(Mwc32xxa8State *obj, uint32_t seed)
{
    obj->x[0] = (uint8_t) (seed & 0xFF);
    obj->x[1] = (uint8_t) ((seed >> 8) & 0xFF);
    obj->x[2] = (uint8_t) ((seed >> 16) & 0xFF);
    obj->c = (uint8_t) ( ((seed >> 24) & 0x7F) | 0x1 );
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}

static void *create(const CallerAPI *intf)
{
    Mwc32xxa8State *obj = intf->malloc(sizeof(Mwc32xxa8State));
    Mwc32xxa8State_init(obj, intf->get_seed32());
    return obj;
}


MAKE_UINT32_PRNG("Mwc32xxa8", NULL)
