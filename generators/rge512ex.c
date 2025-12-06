// https://doi.org/10.5281/zenodo.17713219
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[8];
    uint64_t ctr;
    int pos;
} RGE512State;



static void RGE512State_next(RGE512State *obj)
{
    uint64_t *s = obj->s;    
    const uint64_t ctr = obj->ctr;
    obj->s[0] += ctr;
    obj->ctr += 0x9E3779B97F4A7C15;
    for (int i = 0; i < 2; i++) {
        s[0] += s[1]; s[1] ^= rotl64(s[0], 3);
        s[2] += s[3]; s[3] ^= rotl64(s[2], 12);
        s[4] += s[5]; s[5] ^= rotl64(s[4], 24);    
        s[6] += s[7]; s[7] ^= rotl64(s[6], 48);

        s[5] ^= s[0]; s[0] += rotl64(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl64(s[6], 17);
        s[7] ^= s[2]; s[2] += rotl64(s[7], 23);
        s[4] ^= s[3]; s[3] += rotl64(s[4], 51);
    }
}



static inline uint64_t get_bits_raw(void *state)
{
    RGE512State *obj = state;
    if (obj->pos >= 8) {
        RGE512State_next(obj);
        obj->pos = 0;
    }
    uint64_t out = obj->s[obj->pos++];
    return out;
}

static void *create(const CallerAPI *intf)
{
    RGE512State *obj = intf->malloc(sizeof(RGE512State));
    for (int i = 0; i < 8; i++) {
        obj->s[i] = intf->get_seed64();
    }
    obj->pos = 8;
    obj->ctr = intf->get_seed64();
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    intf->printf("Diffusion demo\n");
    RGE512State *obj = intf->malloc(sizeof(RGE512State));
    for (int i = 0; i < 8; i++) {
        obj->s[i] = 0;
    }
    obj->pos = 8;
    obj->ctr = 1;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            const uint64_t u = get_bits_raw(obj);
            intf->printf("0x%16.16" PRIX64 " ", u);
        }
        intf->printf("\n");
    }
    
    return 1;
}

MAKE_UINT64_PRNG("RGE512ex", run_self_test)
