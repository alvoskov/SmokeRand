#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t u; ///< 64-bit LCG state
    uint64_t v; ///< xorshift state
    uint64_t w; ///< MWC state
} RanState;


static inline uint64_t get_bits_raw(void *state)
{
    RanState *obj = state;
    /* 64-bit LCG part */
    obj->u = obj->u * 2862933555777941757ull + 7046029254386353087ull;
    /* xorshift32 part */
    obj->v ^= obj->v >> 17;
    obj->v ^= obj->v << 31;
    obj->v ^= obj->v >> 8;
    /* MWC part */
    obj->w = 4294957665u * (obj->w & 0xffffffff) + (obj->w >> 32);
    /* Output function (LCG is scrambled) */
    uint64_t x = obj->u ^ (obj->u << 21);
    x ^= x >> 35;
    x ^= x << 4;
    return (x + obj->v) ^ obj->w;
}


void *create(const CallerAPI *intf)
{
    RanState *obj = intf->malloc(sizeof(RanState));
    uint64_t seed = intf->get_seed64();
    obj->v = 4101842887655102017ull;
    obj->w = 1;
    if (seed != 0) {
        seed ^= obj->v;
    }
    obj->u = seed ^ obj->v; (void) get_bits_raw(obj);
    obj->v = obj->u; (void) get_bits_raw(obj);
    obj->w = obj->v; (void) get_bits_raw(obj);
    return obj;
}

MAKE_UINT64_PRNG("Ran", NULL)
