/**
 * @file konadare192.c
 * @brief Konadare192 is a chaotic generator based on an invertible
 * nonlinear mapping and a discrete Weyl sequence.
 * @details The original algorithm was developed by Pelle Evensen
 * and translated into C++ by Ulf Benjaminsson. Refernces:
 *
 * 1. https://github.com/pellevensen/PReenactiNG (not active)
 * 2. https://github.com/ulfben/cpp_prngs
 *
 * @copyright The konadare192 algorithm was developed by Pelle Evensen and
 * archived and translated to C++ by Ulf Benjaminsson.
 *
 * Reentrant C99 implementation for SmokeRand:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t a; ///< Counter
    uint64_t b; ///< Nonlinear mixer part 1
    uint64_t c; ///< Nonlinear mixer part 2
} Konadare192State;


static inline uint64_t get_bits_raw(Konadare192State *obj)
{
    const uint64_t out = obj->b ^ obj->c;
    const uint64_t a0 = obj->a ^ (obj->a >> 32);
    obj->a += 0xBB67AE8584CAA73BU;
    obj->b = rotl64(obj->b + a0, 53); // 53 = 64 - 11
    obj->c = rotl64(obj->c + obj->b, 8);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Konadare192State *obj = intf->malloc(sizeof(Konadare192State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT64_PRNG("konadare192", NULL)
