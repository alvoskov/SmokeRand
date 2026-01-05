/**
 * @file zibri128ex.c
 * @brief Zibri128ex is a modification of Zibri128 chaotic generator
 * made by A.L. Voskov.
 * @details 
 *
 * References:
 *
 * 1. https://github.com/lemire/testingRNG/issues/17
 * 2. https://github.com/Zibri
 * 3. http://www.zibri.org/
 */
// 
// Modified by A.L.Voskov
// PractRand: >= 2 TiB, >= full
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t s[2]; ///< Nonlinear part, "mixer".
    uint64_t ctr;  ///< Linear part, "discrete Weyl sequence'.
} Zibri128ExState;


static uint64_t get_bits_raw(Zibri128ExState *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1];
    obj->ctr += 0x9E3779B97F4A7C15;
    obj->s[0] = rotl64(s0 + s1, 57);
    obj->s[1] = rotl64(s0, 23) + obj->ctr;
    return s0 ^ s1;
}

static void *create(const CallerAPI *intf)
{
    Zibri128ExState *obj = intf->malloc(sizeof(Zibri128ExState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    obj->ctr  = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("Zibri128ex", NULL)
