/**
 * @file biski8.c
 * @brief biski8 is a chaotic generator developed by Daniel Cota.
 * @details Its design resembles one round of Feistel network. Biski8
 * passes the `express` battery bat fails `brief`, `default` and `full`
 * battery. Made mainly for scaled down testing of biski8 mixers,
 * it is too small to be practical. But for a such small state size it
 * is a rather good generator.
 *
 * References:
 * 1. https://github.com/danielcota/biski64
 *
 *
 * @copyright
 * (c) 2025 Daniel Cota (https://github.com/danielcota/biski64)
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t loop_mix;
    uint8_t mix;
    uint8_t ctr;
} Biski8State;



static inline uint8_t Biski8State_get_bits(Biski8State *obj)
{
    uint8_t output = obj->mix + obj->loop_mix;
    uint8_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = (uint8_t) ( ((obj->mix << 2) | (obj->mix >> 6)) + ((old_loop_mix << 5) | (old_loop_mix >> 3)) );
    obj->ctr += 0x99;
    return output;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = Biski8State_get_bits(state);
    }
    return out.u32;
}


static void *create(const CallerAPI *intf)
{
    Biski8State *obj = intf->malloc(sizeof(Biski8State));
    obj->loop_mix = (uint8_t) intf->get_seed64();
    obj->mix = (uint8_t) intf->get_seed64();
    obj->ctr = (uint8_t) intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("biski8", NULL)
