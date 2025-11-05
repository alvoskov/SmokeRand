/**
 * @file ranshi.c
 * @brief
 * @details
 *
 * It seems that `boost_result` variable that is used for making lower
 * bits of `double` output in the original PRNG has a lower quality
 * than the `blk_spin` variable for the upper 32 bits.
 *
 * In its 64-bit form it fails the `hamming_ot` and `hamming_ot_long` tests,
 * also fails `matrixrank` test (but not linear complexity test).
 *
 * 1. https://doi.org/10.1016/0010-4655(95)00005-Z
 * 2. https://geant4.kek.jp/lxr-dev/source/externals/clhep/src/RanshiEngine.cc
 * @copyright 
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    uint64_t seed = intf->get_seed64();
    obj->half_buff = 0;
    obj->red_spin = seed & 0xFFFFFFFF;
    obj->counter = 0;    
    for (size_t i = 0; i < NUMBUFF; i++) {
        seed = 6906969069u * seed + 12345u;
        obj->buffer[i] = (uint32_t) (seed >> 32);
    }
    // Generator warm-up
    for (unsigned long long i = 0; i < NUMBUFF * 128; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("ranshi", NULL)
