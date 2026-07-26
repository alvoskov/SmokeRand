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


static inline uint64_t get_bits_raw(GhSc64State *obj)
{
    if (obj->pos == 4) {
        obj->a = rotl64(obj->a, 25) ^ obj->b;
        obj->b += 11111111111111111;
        obj->c += obj->a;
        obj->d = rotl64(obj->d, 51) + obj->a;
        obj->output[0] = obj->a + obj->b;
        obj->output[1] = rotl64(obj->a, 47) ^ obj->b;
        obj->output[2] = obj->c;
        obj->output[3] = obj->d;
        obj->pos = 0;
    }
    return obj->output[obj->pos++];
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
