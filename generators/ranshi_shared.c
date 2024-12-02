/**
 * @file ranshi_shared.c
 * @brief
 * @details
 * 1. https://doi.org/10.1016/0010-4655(95)00005-Z
 * 2. https://geant4.kek.jp/lxr-dev/source/externals/clhep/src/RanshiEngine.cc
 * @copyright 
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define NUMBUFF 512

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t half_buff;
    uint32_t red_spin;
    uint32_t buffer[NUMBUFF];
    uint32_t counter;
} RanshiState;


static inline uint32_t rotl32(uint32_t x, unsigned int r)
{
    return (x << r) | (x >> (32 - r));
}

static inline uint64_t get_bits_raw(void *state)
{
    RanshiState *obj = state;
    uint32_t red_angle = (((NUMBUFF / 2) - 1) & obj->red_spin) + obj->half_buff;
    uint32_t blk_spin = obj->buffer[red_angle];
    uint32_t boost_result = blk_spin ^ obj->red_spin;
    obj->buffer[red_angle] = rotl32(blk_spin, 17) ^ obj->red_spin;
    obj->red_spin = blk_spin + obj->counter++;
    obj->half_buff = NUMBUFF / 2 - obj->half_buff;
    return (((uint64_t) blk_spin) << 32) | (uint64_t) boost_result;
}


static void *create(const CallerAPI *intf)
{
    RanshiState *obj = intf->malloc(sizeof(RanshiState));
    uint32_t seed = intf->get_seed32();
    obj->half_buff = 0;
    obj->red_spin = seed;
    obj->counter = 0;
    for (int i = 0; i < NUMBUFF; i++) {
        obj->buffer[i] = seed;
    }
    // Generator warm-up
    for (int i = 0; i < NUMBUFF * 32; i++) {
        (void) get_bits_raw(obj);
    }
    return (void *) obj;
}


MAKE_UINT64_PRNG("ranshi", NULL)
