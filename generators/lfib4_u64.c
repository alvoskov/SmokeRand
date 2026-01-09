/**
 * @file lfib4_u64.c 
 * @brief A modification LFIB4 generator by G.Marsaglia that uses 64-bit values
 * and returns only upper 32 bits of these values. Has much higher quality than
 * the original 32-bit generator.
 * @details References:
 *
 * 1. http://www.cse.yorku.ca/~oz/marsaglia-rng.html
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t t[256];
    uint8_t c;
} LFib4U64State;

static inline uint64_t get_bits_raw(LFib4U64State *obj)
{
    uint64_t *t = obj->t;
    obj->c++;
    const uint8_t c1 = (uint8_t) (obj->c + 58U);
    const uint8_t c2 = (uint8_t) (obj->c + 119U);
    const uint8_t c3 = (uint8_t) (obj->c + 178U);
    t[obj->c] += t[c1] + t[c2] + t[c3];
    return t[obj->c] >> 32;
}

static void *create(const CallerAPI *intf)
{
    LFib4U64State *obj = intf->malloc(sizeof(LFib4U64State));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int i = 0; i < 256; i++) {
        obj->t[i] = pcg_bits64(&state);
    }
    obj->c = 0;
    return obj;
}


MAKE_UINT32_PRNG("LFib4_u64", NULL)
