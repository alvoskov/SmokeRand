/**
 * @file xabc16.c
 * @brief A 16-bit modification of nonlinear XABC generator by Daniel Dunn.
 * @details The next modifications were made:
 *
 * 1. Variables were replaced to 16-bit ones.
 * 2. The right shift was replaced to the right rotation.
 * 3. Increment was replaced to the discrete Weyl sequence.
 * 4. An output function was added.
 * 
 * The generator does passes `express` and `brief` batteries battery but fails
 * some tests in the `default` and `full` batteries. It is worse than e.g. SFC16.
 *
 * References:
 *
 * 1. Daniel Dunn (aka EternityForest) The XABC Random Number Generator
 *    https://eternityforest.com/doku/doku.php?id=tech:the_xabc_random_number_generator
 * 2. https://codebase64.org/doku.php?id=base:x_abc_random_number_generator_8_16_bit
 * 3. https://www.electro-tech-online.com/threads/ultra-fast-pseudorandom-number-generator-for-8-bit.124249/
 * 4. https://www.stix.id.au/wiki/Fast_8-bit_pseudorandom_number_generator
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint16_t x;
    uint16_t a;
    uint16_t b;
    uint16_t c;
} Xabc16State;



static inline uint16_t get_bits16(Xabc16State *obj)
{
    obj->a ^= obj->c ^ (obj->x += 0x9E37);
    obj->b += obj->a;
    obj->c = (obj->c + ((obj->b << 11) | (obj->b >> 5))) ^ obj->a;
    return obj->c ^ obj->b;
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = get_bits16(state);
    uint32_t lo = get_bits16(state);

    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xabc16State *obj = intf->malloc(sizeof(Xabc16State));
    uint64_t s = intf->get_seed64();
    obj->a = s & 0xFFFF;
    obj->b = (s >> 16) & 0xFFFF;
    obj->c = (s >> 32) & 0xFFFF;
    obj->x = s >> 48;
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("xabc16", NULL)
