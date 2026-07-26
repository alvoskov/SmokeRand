/**
 * @file ghsc128.c
 * @brief GhostScramble128 - a nonlinear (chaotic) PRNG that has a linear
 * part and its period is at least \f$ 2^64 \f$. Also known as QuarkBurst128.
 * @details See ghsc256.c for details.
 * 
 * References:
 *
 * 1. https://github.com/wprsns/ghostscramble/blob/master/ghostscramble.c
 * 2. https://github.com/eightomic/quarkburst
 * 3. https://www.reddit.com/r/RNG/comments/1ul2fc7/ghostscramble_the_fastest_prng_in_the_universe/
 * 4. https://eightomic.com/
 * 5. https://awesome.ecosyste.ms/projects/github.com%2Fwilliamstaffordparsons%2Fghostscramble
 *
 * @copyright GhostScramble/QuarkBurst PRNG family was developed by
 * William Stafford Parsons.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief GhostScramble128/QuarkBurst128 PRNG state.
 */
typedef struct {
    uint64_t output[2];
    uint64_t a;
    uint64_t b;
    uint64_t c;
    int pos;
} GhSc128State;


static inline uint64_t get_bits_raw(GhSc128State *obj)
{
    if (obj->pos == 2) {
        obj->a = rotl64(obj->a, 29) ^ obj->b;
        obj->b += 11111111111111111;
        obj->output[0] = obj->a + obj->c;
        obj->c = rotl64(obj->c, 43) + obj->a;
        obj->output[1] = (obj->a + obj->b) ^ obj->c;
        obj->pos = 0;
    }
    return obj->output[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
    GhSc128State *obj = intf->malloc(sizeof(GhSc128State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    obj->pos = 2;    
    return obj;
}

MAKE_UINT64_PRNG("GhostScramble128", NULL)
