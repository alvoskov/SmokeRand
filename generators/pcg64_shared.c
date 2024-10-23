// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)


#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Pcg64State;


// pcg_rxs_m_xs64
static inline uint64_t get_bits_raw(void *state)
{
    Pcg64State *obj = state;
    uint64_t word = ((obj->x >> ((obj->x >> 59) + 5)) ^ obj->x) *
        12605985483714917081ull;
    obj->x = obj->x * 6364136223846793005ull + 1442695040888963407ull;
    return (word >> 43) ^ word;
}

static void *create(const CallerAPI *intf)
{
    Pcg64State *obj = intf->malloc(sizeof(Pcg64State));
    obj->x = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("PCG64", NULL)
