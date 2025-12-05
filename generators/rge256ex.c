// Original: https://doi.org/10.5281/zenodo.17713219
// express, brief, default, full - passed
// PractRand: >= 1 TiB
// TestU01 >= Crush
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[8];
    uint64_t ctr;
    int pos;
} RGE256State;



static void RGE256State_next(RGE256State *obj)
{
    uint32_t *s = obj->s;    
    const uint64_t ctr = obj->ctr;
    obj->s[0] += (uint32_t) ctr;
    obj->s[1] += (uint32_t) (ctr >> 32);
    obj->ctr += 0x9E3779B97F4A7C15;
    for (int i = 0; i < 2; i++) {
        s[0] += s[1]; s[1] ^= s[0];
        s[2] += s[3]; s[3] ^= rotl32(s[2], 6);
        s[4] += s[5]; s[5] ^= rotl32(s[4], 12);    
        s[6] += s[7]; s[7] ^= rotl32(s[6], 18);

        s[5] ^= s[0]; s[0] += rotl32(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl32(s[6], 11);
        s[7] ^= s[2]; s[2] += rotl32(s[7], 13);
        s[4] ^= s[3]; s[3] += rotl32(s[4], 17);
    }
}



static inline uint64_t get_bits_raw(void *state)
{
    RGE256State *obj = state;
    if (obj->pos >= 8) {
        RGE256State_next(obj);
        obj->pos = 0;
    }
    uint32_t out = obj->s[obj->pos++];
    return out;
}

static void *create(const CallerAPI *intf)
{
    RGE256State *obj = intf->malloc(sizeof(RGE256State));
    seeds_to_array_u32(intf, obj->s, 8);
    obj->pos = 8;
    obj->ctr = 0;
    return obj;
}

MAKE_UINT32_PRNG("RGE256ex", NULL)
