/**
 * @file ghsc256.c
 * @brief GhostScramble256 - a nonlinear (chaotic) PRNG that has a linear
 * part and its period is at least \f$ 2^64 \f$. Also known as QuarkBurst256.
 * @details It consists of three parts:
 *
 * 1. Counter or "discrete Weyl sequence"
 * 2. Nonlinear reversible mixer.
 * 3. Nonlinear output function.
 *
 * This PRNG can be represented as the next scheme:
 *
 *       a     b     c     d    {O0_A} {O0_C} {O1_B} {O1_C} {O2_D} {O3_D}
 *       |     |     |     |      |      |      |      |      |      |
 *      <<<29  |     |     |      .--------------------------{^}    <<<21
 *       |     |     |     |      |      |      |      |      |      |
 *      {^}----.     |     |      .---------------------------------{+}
 *       |     |     |     |      |      |      |      |      |      |
 *       |    {+}-W  |     |      .------------{+}     |      |      |
 *       |     |     |     |      |      |      |      |      |      |
 *     {O0_A}  |   {O0_C}  |      |      |      |      |      |      |
 *       |     |     |     |     {+}-----.     {^}-----.      |      |
 *       |     |    <<<47  |      |             |             |      |
 *       |    {O1_B} |     |      |             |             |      |
 *       |     |     |     |     {O0}          {O1}         {O2}    {O3}
 *       .----------{+}    |
 *       |     |     |     |
 *       |     |    {O1_C} |
 *       |     |     |     |
 *       |     |     |   {O2_D}
 *       |     |     |     |
 *       |     |     |   <<<25
 *       |     |     |     |
 *       .----------------{+}
 *       |     |     |     |
 *       |     |     |   {O3_D}
 *       |     |     |     |
 *       a     b     c     d
 *
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
 * @brief GhostScramble256/QuarkBurst256 PRNG state.
 */
typedef struct {
    uint64_t output[4];
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    int pos;
} GhSc256State;


static inline uint64_t get_bits_raw(GhSc256State *obj)
{
    if (obj->pos == 4) {
        obj->a = rotl64(obj->a, 29) ^ obj->b;
        obj->b += 111111111111111111;
        obj->output[0] = obj->a + obj->c;
        obj->c = rotl64(obj->c, 47) + obj->a;
        obj->output[1] = (obj->a + obj->b) ^ obj->c;
        obj->output[2] = obj->a ^ obj->d;
        obj->d = rotl64(obj->d, 25) + obj->a;
        obj->output[3] = obj->a + rotl64(obj->d, 21);
        obj->pos = 0;
    }
    return obj->output[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
    GhSc256State *obj = intf->malloc(sizeof(GhSc256State));
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

MAKE_UINT64_PRNG("GhostScramble256", NULL)
