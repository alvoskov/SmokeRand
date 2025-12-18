// https://github.com/danielcota/biski64/tree/main
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define GR 0x9E37U

typedef struct {
    uint16_t last_mix;
    uint16_t mix;
    uint16_t ctr;
} Biski16State;



// biski64 generator function
static inline uint16_t Biski16State_get_bits(Biski16State *obj)
{
    const uint16_t output = (uint16_t) (GR * obj->mix);
    const uint16_t old_rot = rotl16(obj->last_mix, 11);
    obj->last_mix = (uint16_t) (obj->ctr ^ obj->mix);
    obj->mix = (uint16_t) (old_rot + output);
    obj->ctr = (uint16_t) (obj->ctr + GR);
    return output;
}


static inline uint64_t get_bits_raw(void *state)
{
    const uint32_t hi = Biski16State_get_bits(state);
    const uint32_t lo = Biski16State_get_bits(state);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    Biski16State *obj = intf->malloc(sizeof(Biski16State));
    obj->last_mix = (uint16_t) intf->get_seed64();
    obj->mix = (uint16_t) intf->get_seed64();
    obj->ctr = (uint16_t) intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("biski16_mul", NULL)
