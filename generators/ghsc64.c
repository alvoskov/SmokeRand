/**
 * @file ghsc64.c
 * @brief GhostScramble64 - a nonlinear (chaotic) PRNG that has a linear
 * part and its period is at least \f$ 2^64 \f$. Also known as QuarkBurst64.
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
 * @brief GhostScramble64/QuarkBurst64 PRNG state.
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} GhSc64State;


static inline uint64_t get_bits_raw(GhSc64State *obj)
{
    obj->a = rotl64(obj->a, 29) ^ obj->b;
    obj->b += 1111111111111111;
    obj->c = rotl64(obj->c, 41) + obj->a;
    return obj->c;
}


static void *create(const CallerAPI *intf)
{
    GhSc64State *obj = intf->malloc(sizeof(GhSc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("GhostScramble64", NULL)
