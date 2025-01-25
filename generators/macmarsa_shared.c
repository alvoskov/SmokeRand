/**
 * @file macmarsa_shared.c
 * @brief An implementation of MacLaren-Marsaglia generator.
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 32-b
 */
typedef struct {
    uint32_t x; ///< 69069
    uint32_t y; ///< MWC
    uint32_t u[257];
} MMState;


static inline uint32_t lcg32(MMState *obj)
{
    return obj->x = 69069 * obj->x + 12345;
}

static inline uint64_t get_bits_raw(void *state)
{
    MMState *obj = state;
    obj->y = 63885 * (obj->y & 0xFFFF) + (obj->y >> 16);
    int i = obj->y % 257;
    uint32_t out = obj->u[i];
    obj->u[i] = lcg32(obj);
    return out;
}


static void *create(const CallerAPI *intf)
{
    MMState *obj = intf->malloc(sizeof(MMState));
    obj->x = intf->get_seed32();
    obj->y = (intf->get_seed32() & 0xFFFF) | 0x10000;
    for (int i = 0; i < 257; i++) {
        obj->u[i] = lcg32(obj);
    }
    return (void *) obj;
}


MAKE_UINT32_PRNG("MacMarsa", NULL)
