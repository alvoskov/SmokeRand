// https://github.com/lemire/testingRNG/issues/17
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[2];
} Zibri128State;


static uint64_t get_bits_raw(Zibri128State *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1];
    const uint64_t result = rotl64(s0 + s1, 56);
    obj->s[0] = result;
    obj->s[1] = s0;
    return result;
}

static void *create(const CallerAPI *intf)
{
    Zibri128State *obj = intf->malloc(sizeof(Zibri128State));
    obj->s[0] = 0x9E3779B97F4A7C15;
    obj->s[1] = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Zibri128", NULL)
