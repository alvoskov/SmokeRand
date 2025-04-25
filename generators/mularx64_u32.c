/**
 * @file mularx64_u32.c
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
    union {
        uint32_t u32[2];
        uint64_t u64;
    } ctr;
    uint32_t out[2];
    int pos;
} Mularx64x32State;


static inline void mulbox64(uint32_t *v, int i, int j, uint32_t a, int r1, int r2)
{
    uint64_t mul = ((uint64_t) a) * (v[i] ^ v[j]);
    v[i] = (uint32_t) mul;
    v[j] ^= mul >> 32;
    v[j] = v[j] + rotl32(v[i], r1);
    v[i] = v[i] + rotl32(v[j], r2);
}


static inline uint64_t get_bits_raw(void *state)
{
    Mularx64x32State *obj = state;
    if (obj->pos == 2) {
        obj->pos = 0;
        for (int i = 0; i < 2; i++) {
            obj->out[i] = obj->ctr.u32[i];
        }
        mulbox64(obj->out, 0, 1, 0xDCD34D59, 6, 2);
        mulbox64(obj->out, 0, 1, 0xF22B8767, 24, 23);
        mulbox64(obj->out, 0, 1, 0xC4CA0101, 18, 17);
        obj->ctr.u64++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx64x32State *obj = intf->malloc(sizeof(Mularx64x32State));
    obj->pos = 2;
    obj->ctr.u64 = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("Mularx64_u32", NULL)
