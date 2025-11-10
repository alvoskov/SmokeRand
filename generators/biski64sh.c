// does pass `express`, `brief`, `default` and `full` batteries.
// https://github.com/danielcota/biski64/tree/main
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define GR 0x9e3779b97f4a7c15U

typedef struct {
    uint64_t last_mix;
    uint64_t mix;
    uint64_t ctr;
} Biski64State;



// biski64 generator function
static inline uint64_t get_bits_raw(void *state)
{
    Biski64State *obj = state;
    uint64_t output = GR * obj->mix;
    uint64_t old_rot = rotl64(obj->last_mix, 39);
    obj->last_mix = obj->ctr ^ obj->mix;
    obj->mix = old_rot + output;
    obj->ctr += GR;
    return output;
}


static void *create(const CallerAPI *intf)
{
    Biski64State *obj = intf->malloc(sizeof(Biski64State));
    obj->last_mix = 0;
    obj->mix = 0;
    obj->ctr = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("biski64sh", NULL)
