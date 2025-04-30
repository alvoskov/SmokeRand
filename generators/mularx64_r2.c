/**
 * @file mularx64_r2.c
 * @brief A simple counter-based generator.
 * @details It passes BigCrush in the "interleaved" mode, i.e. passing 32-bit
 * chunks as separate values.
 *
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
    union {
        uint32_t u32[2];
        uint64_t u64;
    } out;
} Mularx64x32State;


static inline void mulbox64(uint32_t *v, int i, int j, uint32_t a, int r1, int r2)
{
    uint64_t mul = ((uint64_t) a) * (v[i] ^ v[j]);
    v[i] = (uint32_t) mul;
    v[j] ^= mul >> 32;
    v[i] = v[i] + rotl32(v[j], r1);
    v[j] = v[j] + rotl32(v[i], r2);    
}

static inline uint64_t get_bits_raw(void *state)
{
    Mularx64x32State *obj = state;
    obj->ctr.u64++;
    obj->out.u64 = obj->ctr.u64;

    mulbox64(obj->out.u32, 0, 1, 0xD7474D0B, 30, 6);
    mulbox64(obj->out.u32, 0, 1, 0xE293A7BD, 26, 23);

    return obj->out.u64;
}

static void *create(const CallerAPI *intf)
{
    Mularx64x32State *obj = intf->malloc(sizeof(Mularx64x32State));
    obj->ctr.u64 = intf->get_seed32();
    obj->out.u64 = 0;
    return obj;
}

MAKE_UINT64_PRNG("Mularx64_r2", NULL)
