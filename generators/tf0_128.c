/*
x = 123456789;
for i in range(10001):
    x = (x + (x*x | 5)) % 2**128
print(hex(x // 2**64), hex(x))
https://doi.org/10.1007/3-540-36400-5_34
https://doi.org/10.1007/978-3-540-24654-1_18
*/

#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG


typedef Lcg128State Tf0128State;


static inline uint64_t get_bits_raw(Tf0128State *obj)
{
    //obj->x = obj->x + (obj->x * obj->x | (2**63 + 5));
    uint64_t hi = obj->x_high, lo = obj->x_low;
    umuladd_128x128p64w(obj->x_high, obj->x_low, &hi, &lo, 0);
    lo |= (1ULL << 62) + 5ULL;
    unsigned_add128(&hi, &lo, obj->x_low); hi += obj->x_high;
    obj->x_high = hi; obj->x_low = lo;
    return hi;
}


static void *create(const CallerAPI *intf)
{
    Tf0128State *obj = intf->malloc(sizeof(Tf0128State));
    obj->x_high = intf->get_seed64();
    obj->x_low = intf->get_seed64();
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Tf0128State obj = {.x_low = 123456789, .x_high = 0};
    const uint64_t u_ref = 0x81842387cd7e5265;
    for (int i = 0; i < 10000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint64_t u = get_bits_raw(&obj);
    intf->printf("Out=0x%llX; ref=0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref ? 1 : 0;
    
}

MAKE_UINT64_PRNG("TF0_128", run_self_test)
