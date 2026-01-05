/**
 * @file zibri64ex.c
 * @brief Zibri64ex is a modification of Zibri128 chaotic generator
 * made by A.L. Voskov.
 * @details 
 * >= default
 * References:
 *
 * 1. https://github.com/lemire/testingRNG/issues/17
 * 2. https://github.com/Zibri
 * 3. http://www.zibri.org/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[2]; ///< Nonlinear part, "mixer".
    uint32_t ctr;  ///< Linear part, "discrete Weyl sequence'.
} Zibri64ExState;


static uint64_t get_bits_raw(Zibri64ExState *obj)
{
    const uint32_t s0 = obj->s[0], s1 = obj->s[1];
    obj->s[0] = rotl32(s0 + s1, 23);
    obj->s[1] = rotl32(s0, 7) + (obj->ctr += 0x9E3779B9);
    return s0 ^ s1;
}

static void *create(const CallerAPI *intf)
{
    Zibri64ExState *obj = intf->malloc(sizeof(Zibri64ExState));
    obj->s[0] = intf->get_seed32();
    obj->s[1] = intf->get_seed32();
    obj->ctr  = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("Zibri64ex", NULL)
