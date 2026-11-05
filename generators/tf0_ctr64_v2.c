/**
 * @file tf0_ctr64_v2.c
 * @brief 
 * @details
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    uint64_t x = 180006906969069U * obj->x++;
    x = rotl64(x ^ (x >> 32), 32);
    x += x * x | 0x40000005U; // Tf0 round 1
    x = rotl64(x ^ (x >> 32), 32);    
    x += x * x | 0x40000005U; // Tf0 round 2 (passes full)
    x = rotl64(x ^ (x >> 32), 32);    
    return x;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("tf0_ctr64", NULL)
