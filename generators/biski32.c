// https://github.com/danielcota/biski64
// passes `default`, vulnerable to hamming_distr xor test (128-bit blocks)

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t loop_mix;
    uint32_t mix;
    uint32_t ctr;
} Biski32State;


// biski64 generator function
static inline uint64_t get_bits_raw(void *state)
{
    Biski32State *obj = state;

    uint32_t output = obj->mix + obj->loop_mix;
    uint32_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = rotl32(obj->mix, 8) + rotl32(old_loop_mix, 20);
    obj->ctr += 0x99999999;
    return output;
}


static void *create(const CallerAPI *intf)
{
    Biski32State *obj = intf->malloc(sizeof(Biski32State));
    obj->loop_mix = intf->get_seed32();
    obj->mix = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("biski32", NULL)
