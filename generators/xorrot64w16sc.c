// (5,5,11)/1e-42, (7,6,9)/1e-11, (5,7,11)/1e-5, (8,3,14)/1e-29
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t w;
} Xorrot64w16State;

static inline uint16_t get_bits16(Xorrot64w16State *obj)
{
    const uint16_t x0 = obj->x, w0 = obj->w;
    const uint16_t out = (uint16_t) (rotl16(x0 - w0, 2) - w0);
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = (uint16_t) (x0 ^ (x0 << 5) ^ w0 ^ rotl16(w0, 7) ^ rotl16(w0, 11));
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = get_bits16(state);
    const uint32_t lo = get_bits16(state);
    return (hi << 16) | lo;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64w16State *obj = intf->malloc(sizeof(Xorrot64w16State));
    const uint64_t seed = intf->get_seed64();
    obj->x = (uint16_t) seed;
    obj->y = (uint16_t) (seed >> 16);
    obj->z = (uint16_t) (seed >> 32);
    obj->w = (uint16_t) (seed >> 48);
    if (obj->x == 0 && obj->y == 0 && obj->z == 0 && obj->w == 0) {
        obj->x = 0xBEEF;
    }
    return obj;
}


MAKE_UINT32_PRNG("xorrot64w16--", NULL)


