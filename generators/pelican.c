// https://github.com/tommyettinger/sarong/blob/master/src/main/java/sarong/PelicanRNG.java
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    uint64_t n = obj->x++;
    n ^= rotl64(n, 17) ^ rotl64(n, 41) ^ 0xD1B54A32D192ED03U;
    n *= 0xAEF17502108EF2D9L;
    n ^= (n >> 23) ^ (n >> 31) ^ (n >> 43);
    n *= 0xDB4F0B9175AE2165L;
    n ^= n >> 28;
    return n;
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT64_PRNG("pelican", NULL)
