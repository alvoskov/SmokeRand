#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x[8];
    uint64_t c;
    unsigned int i;
} Rwc64LargeState;


// x_n = x_{n-1} + ax_{n-8} + c_{n-1}
static inline uint64_t get_bits_raw(Rwc64LargeState *obj)
{
    static const uint64_t a = 12132979027010582507U;
    const unsigned int j = (obj->i + 1) & 0x7;
    const uint64_t x = obj->x[obj->i], y = obj->x[j]; // x_{n-1}, x_{n-8}
    uint64_t new_hi, new_lo;
    new_lo = unsigned_muladd128(a, y, x, &new_hi);
    unsigned_add128(&new_hi, &new_lo, obj->c);
    obj->x[j] = new_lo;
    obj->c = new_hi;
    obj->i = j;
    return new_lo;
}


static void *create(const CallerAPI *intf)
{
    Rwc64LargeState *obj = intf->malloc(sizeof(Rwc64LargeState));
    seeds_to_array_u64(intf, obj->x, 8);
    obj->c = 1;
    obj->i = 6; // r_(i-1); note; r_(i-8) will be at pos 7
    // Warmup for possible stream decorrelation
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("rwc64large", NULL)
