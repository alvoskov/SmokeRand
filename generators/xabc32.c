/**
 * @file xabc32.c
 * @brief A 32-bit modification of nonlinear XABC generator by Daniel Dunn.
 * @details The next modifications were made:
 *
 * 1. Variables were replaced to 32-bit ones.
 * 2. The right shift was replaced to the right rotation.
 * 3. Increment was replaced to the discrete Weyl sequence.
 * 4. An output function was added.
 * 
 * The generator does passes `full` battery, it is slower than SFC32.
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
    uint32_t x;
    uint32_t a;
    uint32_t b;
    uint32_t c;
} Xabc32State;



static inline uint64_t get_bits_raw(void *state)
{
    Xabc32State *obj = state;    
    obj->a ^= obj->c ^ (obj->x += 0x9E3779B9);
    obj->b += obj->a;
    obj->c = (obj->c + rotr32(obj->b, 9)) ^ obj->a;
    return obj->c ^ obj->b;

}

static void *create(const CallerAPI *intf)
{
    Xabc32State *obj = intf->malloc(sizeof(Xabc32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->c = intf->get_seed32();
    obj->x = intf->get_seed32();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("xabc32", NULL)
