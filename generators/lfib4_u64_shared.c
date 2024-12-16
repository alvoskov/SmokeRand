/**
 * @file lfib4_u6_shared.c 
 * @brief A modification LFIB4 generator by G.Marsaglia that uses 64-bit values
 * and returns only upper 32 bits of these values. Has much higher quality than
 * the original 32-bit generator.
 * @details References:
 *
 * 1. http://www.cse.yorku.ca/~oz/marsaglia-rng.html
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t t[256];
    uint8_t c;
} LFib4U64State;

static inline uint64_t get_bits_raw(void *state)
{
    LFib4U64State *obj = state;
    uint64_t *t = obj->t;
    obj->c++;
    uint8_t c1 = obj->c + 58, c2 = obj->c + 119, c3 = obj->c + 178;
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
    return (void *) obj;
}


MAKE_UINT32_PRNG("LFib4_u64", NULL)
