/**
 * @brief GhostScramble non-linear PRNG
 * @details
 *
 * 1. https://www.reddit.com/r/RNG/comments/1ul2fc7/ghostscramble_the_fastest_prng_in_the_universe/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t output[4];
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    int pos;
} GhSc64State;


static inline uint64_t get_bits_raw(GhSc64State *s)
{
    if (s->pos == 4) {
        s->a = rotl64(s->a, 25) ^ s->b;
        s->b += 11111111111111111;
        s->c += s->a;
        s->d = rotl64(s->d, 51) + s->a;
        s->output[0] = s->a + s->b;
        s->output[1] = rotl64(s->a, 47) ^ s->b;
        s->output[2] = s->c;
        s->output[3] = s->d;
        s->pos = 0;
    }
    return s->output[s->pos++];
}

static void *create(const CallerAPI *intf)
{
    GhSc64State *obj = intf->malloc(sizeof(GhSc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    obj->d = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    obj->pos = 4;    
    return obj;
}

MAKE_UINT64_PRNG("GhostScramble64Old", NULL)
