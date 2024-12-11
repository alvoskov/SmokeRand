#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t v; ///< xorshift64 state
    uint64_t w; ///< MWC state
} RanQ2State;

static inline uint64_t get_bits_raw(void *state)
{
    RanQ2State *obj = state;
    obj->v ^= obj->v >> 17;
    obj->v ^= obj->v << 31;
    obj->v ^= obj->v >> 8;
    obj->w = 4294957665U * (obj->w & 0xFFFFFFFF) + (obj->w >> 32);
    return obj->w ^ obj->v;
}

void *create(const CallerAPI *intf)
{
    RanQ2State *obj = intf->malloc(sizeof(RanQ2State));
    obj->v = intf->get_seed64();
    if (obj->v == 0) {
        obj->v = 4101842887655102017ULL;
    }
    obj->v = intf->get_seed32() | (1ull << 32);
    return obj;
}

MAKE_UINT64_PRNG("RanQ2", NULL)
