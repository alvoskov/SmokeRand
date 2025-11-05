// https://github.com/danielcota/biski64/tree/main
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    GR = 0x9d
};

typedef struct {
    uint8_t last_mix;
    uint8_t mix;
    uint8_t ctr;
} Biski8State;



// biski64 generator function
static inline uint8_t Biski8State_get_bits(Biski8State *obj)
{
    uint8_t output = GR * obj->mix;
    uint8_t old_rot = (uint8_t) ((obj->last_mix << 3) | (obj->last_mix >> 5));
    obj->last_mix = obj->ctr ^ obj->mix;
    obj->mix = old_rot + output;
    obj->ctr += GR;
    return output;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = Biski8State_get_bits(state);
    }
    return out.u32;
}

static void *create(const CallerAPI *intf)
{
    Biski8State *obj = intf->malloc(sizeof(Biski8State));
    obj->last_mix = (uint8_t) intf->get_seed64();
    obj->mix = (uint8_t) intf->get_seed64();
    obj->ctr = (uint8_t) intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("biski16_mul", NULL)
