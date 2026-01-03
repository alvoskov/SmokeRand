/**
 * @file xorrot32.c
 * @brief xorrot32 is a LFSR with 32-bit state, its period is \f$2^{32} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1].
 *
 * References:
 *
 * 1. Ronald L. Rivest. On the invertibility of the XOR of rotations of
 *    a binary word https://people.csail.mit.edu/rivest/pubs/Riv11e.prepub.pdf
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x;
} Xorrot32State;


static inline uint64_t get_bits_raw(Xorrot32State *obj)
{
    obj->x ^= obj->x << 1;
    obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Xorrot32State *obj = intf->malloc(sizeof(Xorrot32State));
    obj->x = intf->get_seed32();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return (void *) obj;
}


MAKE_UINT32_PRNG("xorrot32", NULL)
