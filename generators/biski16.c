// https://github.com/danielcota/biski64/tree/main
// 0/0/0/1
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t loop_mix;
    uint16_t mix;
    uint16_t ctr;
} Biski16State;


// biski64 generator function
static inline uint16_t Biski16State_get_bits(Biski16State *obj)
{
    uint16_t output = obj->mix + obj->loop_mix;
    uint16_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl16(obj->mix, 4) + rotl16(old_loop_mix, 9);
    obj->ctr += 0x9999;
    return output;
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t hi = Biski16State_get_bits(state);
    uint32_t lo = Biski16State_get_bits(state);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    Biski16State *obj = intf->malloc(sizeof(Biski16State));
    obj->loop_mix = (uint16_t) intf->get_seed64();
    obj->mix = (uint16_t) intf->get_seed64();
    obj->ctr = (uint16_t) intf->get_seed64();
    return obj;
}

MAKE_UINT32_PRNG("biski16", NULL)
