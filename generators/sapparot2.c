/**
 * @file sapparot2.c
 * @brief Sapparot2 is a chaotic generator.
 * @details References:
 *
 * 1. Ilya O. Levin. Sapparot-2 Fast Pseudo-Random Number Generator.
 *    http://www.literatecode.com/sapparot2
 *
 * WARNING! IT HAS NO GUARANTEED MINIMAL PERIOD! BAD SEEDS ARE POSSIBLE!
 * DON'T USE THIS PRNG FOR ANY SERIOUS WORK! The author uses rotations
 * in the "discrete Weyl sequence" part without proof of a minimal cycle
 * of an updated counter part.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
} Sapparot2State;

#define C_RTR 7
#define C_SH 27
#define PHI 0x9e3779b9

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t m;
    Sapparot2State *obj = state;
    obj->c += obj->a;
    obj->c = rotl32(obj->c, (int) (obj->b >> C_SH));
    obj->b = (obj->b + ((obj->a << 1) + 1)) ^ rotl32(obj->b, 5);
    obj->a += PHI;
    obj->a = rotl32(obj->a, C_RTR);
    m = obj->a;
    obj->a = obj->b;
    obj->b = m;
    return obj->c ^ obj->b ^ obj->a;
}

static void *create(const CallerAPI *intf)
{
    Sapparot2State *obj = intf->malloc(sizeof(Sapparot2State));
    uint64_t seed = intf->get_seed64();    
    obj->a = (uint32_t) (seed >> 32);
    obj->b = (uint32_t) (seed & 0xFFFFFFFF);
    obj->c = intf->get_seed32();
    return (void *) obj;
}

MAKE_UINT32_PRNG("sapparot2", NULL)
