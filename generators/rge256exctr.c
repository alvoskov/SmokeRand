// Original: https://doi.org/10.5281/zenodo.17713219
// express, brief, default, full - passed even with 5 rounds instead of 6
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t ctr[8];
    uint32_t out[8];
    int pos;
} RGE256ExCtrState;




static inline void RGE256ExCtrState_block(RGE256ExCtrState *obj)
{
    uint32_t *s = obj->out;
    for (int i = 0; i < 8; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < 6; i++) { // Even 5 rounds is enough for `full`
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


static void RGE256ExCtrState_init(RGE256ExCtrState *obj, const uint32_t *seed)
{
    obj->ctr[0] = 0;          obj->ctr[1] = 0;
    obj->ctr[2] = 0x243F6A88; obj->ctr[3] = 0x85A308D3;
    obj->ctr[4] = seed[0];    obj->ctr[5] = seed[1];
    obj->ctr[6] = seed[2];    obj->ctr[7] = seed[3];
    RGE256ExCtrState_block(obj);
    obj->pos = 0;
}

static inline uint64_t get_bits_raw(void *state)
{
    RGE256ExCtrState *obj = state;
    if (obj->pos >= 8) {
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        RGE256ExCtrState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    RGE256ExCtrState *obj = intf->malloc(sizeof(RGE256ExCtrState));
    uint32_t seed[4];
    seeds_to_array_u32(intf, seed, 4);
    RGE256ExCtrState_init(obj, seed);
    return obj;
}

MAKE_UINT32_PRNG("RGE256ex-ctr", NULL)
