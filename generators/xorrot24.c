#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t z;
} Xorrot24State;


static inline uint8_t get_byte(Xorrot24State *obj)
{
    const uint8_t x0 = obj->x, z0 = obj->z;
    obj->x = (uint8_t) (x0 ^ obj->y);
    obj->y = (uint8_t) (x0 ^ z0);
    obj->z = (uint8_t) ((x0 << 1) ^ obj->y ^ rotl8(z0, 1) ^ rotl8(z0, 7));
    return (uint8_t) (x0 + z0);
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t out = get_byte(state);
    out |= (uint32_t)get_byte(state) << 8;
    out |= (uint32_t)get_byte(state) << 16;
    out |= (uint32_t)get_byte(state) << 24;
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot24State *obj = intf->malloc(sizeof(Xorrot24State));
    uint32_t seed = intf->get_seed32();
    if (seed == 0) {
        seed = 0xDEADBEEF;
    }
    obj->x = (uint8_t) seed;
    obj->y = (uint8_t) (seed >> 8);
    obj->z = (uint8_t) (seed >> 16);
    return obj;
}

MAKE_UINT32_PRNG("xorrot24", NULL)
