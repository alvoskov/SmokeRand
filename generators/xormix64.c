#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t x;
    uint64_t w;
} Xormix64State;


static inline uint64_t get_bits_raw(Xormix64State *obj)
{
    obj->w += 0x9E3779B97F4A7C15U;
    obj->x = (obj->x ^ rotl64(obj->x, 3) ^ rotl64(obj->x, 29)) + obj->w;
    return obj->x;
}

static void *create(const CallerAPI *intf)
{
    Xormix64State *obj = intf->malloc(sizeof(Xormix64State));
    obj->x = intf->get_seed64();
    obj->w = intf->get_seed64();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("xormix64", NULL)


// 