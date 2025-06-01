/**
 * @file xabc64.c
 * @brief A 64-bit modification of nonlinear XABC generator by Daniel Dunn.
 * @details The next modifications were made:
 *
 * 1. Variables were replaced to 64-bit ones.
 * 2. The right shift was replaced to the right rotation.
 * 3. Increment was replaced to the discrete Weyl sequence.
 * 4. An output function was added.
 * 
 * The generator does passes `full` battery, it is slower than SFC64.
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
    uint64_t x;
    uint64_t a;
    uint64_t b;
    uint64_t c;
} Xabc64State;



static inline uint64_t get_bits_raw(void *state)
{
    Xabc64State *obj = state;    
    obj->a ^= obj->c ^ (obj->x += 0x9E3779B97F4A7C15);
    obj->b += obj->a;
    obj->c = (obj->c + rotr64(obj->b, 12)) ^ obj->a;
    return obj->c ^ obj->b;

}

static void *create(const CallerAPI *intf)
{
    Xabc64State *obj = intf->malloc(sizeof(Xabc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    obj->x = intf->get_seed64();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT64_PRNG("xabc64", NULL)
