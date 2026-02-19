/**
 * @file xorrot64.c
 * @brief xorrot64 is a LFSR with 64-bit state, its period is \f$2^{64} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1]. Seems to be
 * slightly better than the classical xorshift64.
 *
 * Some fairly good triples: (5, 13, 47), (3, 23, 47), (7, 23, 29).
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
    uint64_t x;
} Xorrot64State;


static inline uint64_t get_bits_raw(Xorrot64State *obj)
{
    obj->x ^= obj->x << 5;
    obj->x ^= rotl64(obj->x, 13) ^ rotl64(obj->x, 47);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64State *obj = intf->malloc(sizeof(Xorrot64State));
    obj->x = intf->get_seed64();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return obj;
}


MAKE_UINT64_PRNG("xorrot64", NULL)
