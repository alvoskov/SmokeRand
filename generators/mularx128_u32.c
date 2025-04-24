/**
 * @file mularx128_u32.c
 * @brief A simple counter-based generator that passes `full` battery and
 * 64-bit birthday paradox test.
 *
 * References:
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
 * @brief SplitMix PRNG state.
 */
typedef struct {
    uint32_t x[4];
    uint32_t out[4];
    int pos;
} Mularx128x32State;


static inline void mulbox64(uint32_t *v, int i, int j)
{
    const uint64_t a = 0xf9b25d65ull;
    uint64_t mul = a * (v[i] ^ v[j]);
    v[i] = (uint32_t) mul;
    v[j] ^= mul >> 32;
    v[j] = v[j] + rotl32(v[i], 11);
    v[i] = v[i] ^ rotl32(v[j], 20);
}


static inline void arxbox64(uint32_t *v, int i, int j)
{
    v[j] = v[j] + rotl32(v[i], 11);
    v[i] = v[i] ^ rotl32(v[j], 20);
}


static inline uint64_t get_bits_raw(void *state)
{
    Mularx128x32State *obj = state;
    if (obj->pos == 4) {
        obj->pos = 0;
        for (int i = 0; i < 4; i++) {
            obj->out[i] = obj->x[i];
        }
        obj->out[0] ^= 0x243F6A88;
        mulbox64(obj->out, 0, 1);
        mulbox64(obj->out, 1, 2);
        mulbox64(obj->out, 2, 3);
        mulbox64(obj->out, 3, 0);
        arxbox64(obj->out, 1, 0);
        arxbox64(obj->out, 3, 2);
        obj->x[0]++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx128x32State *obj = intf->malloc(sizeof(Mularx128x32State));
    obj->pos = 4;
    obj->x[0] = obj->x[1] = obj->x[2] = 0;
    obj->x[3] = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("Mularx128_u32", NULL)
