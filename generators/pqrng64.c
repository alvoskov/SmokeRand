// 0x517CC1B727220A95 (mod8 = 5)
// 0x9E3779B97F4A7C13 (mod8 = 3)
// http://www.freecx.co.uk/pqRNG/
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t x;
} PqrngState;


static inline uint64_t get_bits_raw(PqrngState *obj)
{
    static const uint64_t r = 0x517CC1B727220A95; // (mod8 = 5)
    static const uint64_t p = 0x9E3779B97F4A7C13; // (mod8 = 3)
    obj->x = (obj->x ^ r) * p;
    return obj->x >> 32;
}


static void *create(const CallerAPI *intf)
{
    PqrngState *obj = intf->malloc(sizeof(PqrngState));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT32_PRNG("PQRNG64", NULL)
