/**
 * @file lcg64bd.c
 * @brief 64-bit LCG with Bays-Durham shuffle.
 * @details 
 *
 * References:
 * 
 * 1. C. Bays, S. D. Durham. Improving a Poor Random Number Generator //
 *    ACM Transactions on Mathematical Software (TOMS). 1976. V. 2. N 1.
 *    P. 59-64. https://doi.org/10.1145/355666.355670
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define TBL_SIZE 32
#define TBL_INDMASK 0x1F

typedef struct {
    uint64_t lcg;
    uint32_t t[TBL_SIZE];
    uint32_t z;
} Lcg64bdState;


static inline uint32_t lcg64_next(Lcg64bdState *obj)
{
    obj->lcg = 6906969069U * obj->lcg + 1234567U;
    return (uint32_t) (obj->lcg >> 32);
}


static inline uint64_t get_bits_raw(Lcg64bdState *obj)
{
    const int j = obj->z & TBL_INDMASK;
    obj->z = obj->t[j];
    obj->t[j] = lcg64_next(obj);
    return obj->z;
}


static void *create(const CallerAPI *intf)
{                                             
    Lcg64bdState *obj = intf->malloc(sizeof(Lcg64bdState));
    obj->lcg = intf->get_seed64();
    for (int i = 0; i < TBL_SIZE; i++) {
        obj->t[i] = lcg64_next(obj);
    }
    obj->z = lcg64_next(obj);
    return obj;
}

MAKE_UINT32_PRNG("lcg64bd", NULL)
