/**
 * @file xabc8.c
 * @brief A modification of nonlinear XABC generator by Daniel Dunn.
 * @details The next modifications were made:
 *
 * 1. The right shift was replaced to the right rotation.
 * 2. Increment was replaced to the discrete Weyl sequence.
 * 3. An output function was added.
 * 
 * The generator does passes `express` battery but fails a lot of tests
 * in other batteries. It is worse than e.g. SFC8.
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
    uint8_t x;
    uint8_t a;
    uint8_t b;
    uint8_t c;
} Xabc8State;



static inline uint8_t get_bits8(Xabc8State *obj)
{
    obj->x = (uint8_t) (obj->x + 151U);
    obj->a = (uint8_t) (obj->a ^ obj->c ^ obj->x);
    obj->b = (uint8_t) (obj->b + obj->a);
    obj->c = (uint8_t) ( (obj->c + rotl8(obj->b, 7)) ^ obj->a );
    return obj->c ^ obj->b;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = get_bits8(state);
    }
    return out.u32;
}


static void *create(const CallerAPI *intf)
{
    Xabc8State *obj = intf->malloc(sizeof(Xabc8State));
    uint64_t s = intf->get_seed64();
    obj->a = s & 0xFF;
    obj->b = (s >> 8) & 0xFF;
    obj->c = (s >> 16) & 0xFF;
    obj->x = (s >> 24) & 0xFF;
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("xabc8", NULL)
