/**
 * @file xorrot64w16sc.c
 * @brief A scrambled LFSR for 16-bit computers that has the \f$2^{64} - 1\f$
 * period.
 * @details Even without scrambler it fails only 5 tests in the `default`
 * battery and behaves well in the `hamming_distr` test. It uses the next
 * step matrix:
 *
 *                  | 0 0 I A |
 *     |x y z w | * | I 0 0 0 |
 *                  | 0 I 0 0 |
 *                  | 0 0 I B |
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t w;
} Xorrot64w16State;


static inline uint16_t get_bits16(Xorrot64w16State *obj)
{
    const uint16_t x0 = obj->x, w0 = obj->w;
    const uint16_t out = (uint16_t) (rotl16(x0 - w0, 3) - w0);
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (uint16_t) ((x0 << 3) ^ obj->z ^ rotl16(w0, 4) ^ rotl16(w0, 11));
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = get_bits16(state);
    const uint32_t lo = get_bits16(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64w16State *obj = intf->malloc(sizeof(Xorrot64w16State));
    uint64_t seed = intf->get_seed64();
    if (seed == 0) {
        seed = 0x123456789ABCDEF;
    }
    obj->x = (uint16_t) seed;
    obj->y = (uint16_t) (seed >> 16);
    obj->z = (uint16_t) (seed >> 32);
    obj->w = (uint16_t) (seed >> 48);
    return obj;
}


MAKE_UINT32_PRNG("xorrot64w16--", NULL)


