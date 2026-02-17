// 0x517CC1B727220A95 (mod8 = 5)
// 0x9E3779B97F4A7C13 (mod8 = 3)
// http://www.freecx.co.uk/pqRNG/
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG


typedef Lcg128State Pqrng128State;


static inline uint64_t get_bits_raw(Pqrng128State *obj)
{
    static const uint64_t r = 0x517CC1B727220A95; // (mod8 = 5)
    static const uint64_t p = 0x9E3779B97F4A7C13; // (mod8 = 3)
    // obj->x = (obj->x ^ r) * p;
    uint64_t mul0_high;
    obj->x_low ^= r;
    obj->x_high ^= r;
    obj->x_low = unsigned_mul128(p, obj->x_low, &mul0_high);
    obj->x_high = p * obj->x_high + mul0_high;
    return obj->x_high;
}


static void *create(const CallerAPI *intf)
{
    Pqrng128State *obj = intf->malloc(sizeof(Pqrng128State));
    obj->x_high = intf->get_seed64();
    obj->x_low = intf->get_seed64();
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Pqrng128State obj = {.x_low = 123456789, .x_high = 0};
    const uint64_t u_ref = 0xfca6c45fab770809;
    for (int i = 0; i < 10000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint64_t u = get_bits_raw(&obj);
    intf->printf("Out=0x%llX; ref=0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref ? 1 : 0;
    
}


MAKE_UINT64_PRNG("PQRNG128", run_self_test)
