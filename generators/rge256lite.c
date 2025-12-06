// express, default, brief, full
// SmallCrush, Crush, BigCrush
// PractRand: >= 1 TiB
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[8];
    int nrounds;
} RGE256LiteState;


static inline uint32_t RGE256LiteState_next(RGE256LiteState *obj)
{
    uint32_t *s = obj->s;    
    for (int i = 0; i < obj->nrounds; i++) {
        // Quad updates
        s[0] += s[1]; s[1] = rotl32(s[1] ^ s[0], 7);
        s[2] += s[3]; s[3] = rotl32(s[3] ^ s[2], 9);
        s[4] += s[5]; s[5] = rotl32(s[5] ^ s[4], 13);
        s[6] += s[7]; s[7] = rotl32(s[7] ^ s[6], 18);
        // Cross coupling
        s[0] ^= s[4];
        s[1] ^= s[5];
        s[2] ^= s[6];
        s[3] ^= s[7];
    }
    return s[0] ^ s[4];
}


static inline uint64_t get_bits_raw(void *state)
{
    return RGE256LiteState_next(state);
}


static void *create(const CallerAPI *intf)
{
    RGE256LiteState *obj = intf->malloc(sizeof(RGE256LiteState));
    // Seeding
    seeds_to_array_u32(intf, obj->s, 7);
    obj->s[7] = 0x243F6A88; // To prevent bad states
    obj->nrounds = 3;
    // Warmup
    for (int i = 0; i < 10; i++) {
        (void) RGE256LiteState_next(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("RGE256lite", NULL)
