// https://github.com/lemire/testingRNG/issues/17
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[3];
} Zibri192State;


static uint64_t get_bits_raw(Zibri192State *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1], s2 = obj->s[2];
    obj->s[0] = rotl64(s0 + s1 + s2, 48);
    obj->s[1] = s0;
    obj->s[2] = s1;
    return obj->s[0];
}

static void *create(const CallerAPI *intf)
{
    Zibri192State *obj = intf->malloc(sizeof(Zibri192State));
    obj->s[0] = 0x9E3779B97F4A7C15;
    obj->s[1] = intf->get_seed64();
    obj->s[2] = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Zibri192", NULL)
