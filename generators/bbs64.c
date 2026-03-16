/**
 * @file lcg64bd.c
 * @brief BBS64 toy version.
 * @details Passes express, brief and default batteries, 16 GiB in PractRand.
 *
 * References:
 *
 * 1. L. Blum, M. Blum and M. Shub, "A Simple Unpredictable Pseudo-Random Number Generator" SIAM Journal on Computing, Vol. 15, No. 2, 1986, pp. 364-383. doi:10.1137/0215025
 * 2. http://www.ciphersbyritter.com/NEWS2/TESTSBBS.HTM
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Bbs64State;


static inline uint64_t get_bits_raw(Bbs64State *obj)
{
    static const uint64_t pq = 4294967087ULL * 4294957307ULL;
    __uint128_t x = obj->x;
    x = (x * x) % pq;
    obj->x = (uint64_t) x;
    return (uint64_t) x;
}


static void *create(const CallerAPI *intf)
{                                             
    Bbs64State *obj = intf->malloc(sizeof(Bbs64State));
    obj->x = (intf->get_seed32() >> 1) | 0x1;
    return obj;
}

MAKE_UINT64_PRNG("bbs64", NULL)

