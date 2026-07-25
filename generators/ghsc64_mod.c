/**
 * @brief GhostScramble non-linear PRNG (modified by A.L. Voskov)
 * @details
 * express/brief/default/full
 * interleaved32: express/brief/default/full
 * 8 TiB PractRand 0.96 (fails at 16 TiB, BCFN test)
 *
 * 1. https://www.reddit.com/r/RNG/comments/1ul2fc7/ghostscramble_the_fastest_prng_in_the_universe/
 * 2. https://github.com/wprsns/ghostscramble/blob/master/ghostscramble.c
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
        // Mixer
        const uint64_t a = rotl64(obj->a, 25) ^ obj->b;
        obj->b += obj->b * obj->b | 0x40000005; // Klimov-Shamir T-function
        obj->c = rotl64(obj->c, 19) ^ a;
        obj->a = rotl64(obj->d, 51) + a;
        obj->d = a;
        // Output function
        obj->output[0] = obj->a + obj->b;
        obj->output[1] = rotl64(obj->a, 47) + obj->b;
        obj->output[2] = obj->c + obj->d;
        obj->output[3] = rotl64(obj->c, 47) + obj->d;
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

MAKE_UINT64_PRNG("GhostScramble64Mod", NULL)
