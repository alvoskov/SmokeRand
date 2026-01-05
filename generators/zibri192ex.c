// https://github.com/lemire/testingRNG/issues/17
// passes `full`
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[3];
    uint64_t ctr;
} Zibri192ExState;


static uint64_t get_bits_raw(Zibri192ExState *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1], s2 = obj->s[2];
    const uint64_t result = rotl64(s0 + s1 + s2, 51);
    obj->s[0] = result;
    obj->s[1] = rotl64(s0, 17) + (obj->ctr += 0x9E3779B97F4A7C15);
    obj->s[2] = s1;
    return s0 ^ s1;
}

static void *create(const CallerAPI *intf)
{
    Zibri192ExState *obj = intf->malloc(sizeof(Zibri192ExState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    obj->s[2] = intf->get_seed64();
    obj->ctr  = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Zibri192ex", NULL)
