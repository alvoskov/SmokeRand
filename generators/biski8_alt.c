// https://github.com/danielcota/biski64
// 0/17/?/?

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t loop_mix;
    uint8_t mix;
    uint8_t ctr;
} Biski8State;


static inline uint8_t rotl8(uint8_t x, int r)
{
    return (uint8_t) ( (x << r) | (x >> (8 - r)) );
}



static inline uint8_t Biski8State_get_bits(Biski8State *obj)
{
    uint8_t output = obj->mix + obj->loop_mix;
    uint8_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = (uint8_t) (obj->ctr ^ obj->mix);
    obj->mix = (uint8_t) ( (obj->mix ^ rotl8(obj->mix, 2)) + rotl8(old_loop_mix, 5) );
    obj->ctr += 0x99;
    return output;
}


// biski64 generator function
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
    obj->loop_mix = (uint8_t) intf->get_seed64();
    obj->mix = (uint8_t) intf->get_seed64();
    obj->ctr = (uint8_t) intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("biski8_alt", NULL)
