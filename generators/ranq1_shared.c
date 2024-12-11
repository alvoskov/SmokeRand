#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t v; ///< xorshift64 state
} RanQ1State;

static inline uint64_t get_bits_raw(void *state)
{
    RanQ1State *obj = state;
    obj->v ^= obj->v >> 21;
    obj->v ^= obj->v << 35;
    obj->v ^= obj->v >> 4;
    return obj->v * 2685821657736338717ULL;
}

void *create(const CallerAPI *intf)
{
    RanQ1State *obj = intf->malloc(sizeof(RanQ1State));
    uint64_t seed = intf->get_seed64();
    obj->v = 4101842887655102017ULL;
    if (seed != obj->v) {
        obj->v ^= seed;
    }
    (void) get_bits_raw(obj);
    return obj;
}

MAKE_UINT64_PRNG("RanQ1", NULL)
