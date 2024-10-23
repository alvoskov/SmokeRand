/**
 * @file shr3_shared.c
 * @brief An implementation of SHR3 - classic 32-bit LSFR generator
 * proposed by G. Marsaglia.
 * @details Fails almost all statistical tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RANDU PRNG state
 */
typedef struct {
    uint32_t x;
} SHR3State;


static inline uint64_t get_bits_raw(void *state)
{
    SHR3State *obj = state;
    obj->x ^= (obj->x << 17);
    obj->x ^= (obj->x >> 13);
    obj->x ^= (obj->x << 5);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    SHR3State *obj = intf->malloc(sizeof(SHR3State));
    obj->x = intf->get_seed32() | 0x1;
    return (void *) obj;
}


MAKE_UINT32_PRNG("SHR3", NULL)
