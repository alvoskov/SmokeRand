// https://gist.github.com/andanteyk/916c48ea34d7e6d18fd261c22ba69e4b
// This is a modified version of xoshiro256++, with changes to the shift/rotate constants and the XOR indices.
// In fact, it is about 4-6% faster compared to the original.
// The period is guaranteed to be the same as that of the original.
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief PRNG state.
 */
typedef struct {
    uint64_t s[4];
} Xoshiro256PPState;

static inline uint64_t get_bits_raw(Xoshiro256PPState *state)
{
    const uint64_t result = rotl64(state->s[0] + state->s[3], 23) + state->s[0];
    const uint64_t t = state->s[3] << 3;
    state->s[3] ^= state->s[0];
    state->s[0] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[2] ^= state->s[3];

    state->s[0] ^= t;
    state->s[2] = rotl64(state->s[2], 45);
    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoshiro256PPState *obj = intf->malloc(sizeof(Xoshiro256PPState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    obj->s[2] = intf->get_seed64();
    obj->s[3] = intf->get_seed64();
    if (obj->s[0] == 0 && obj->s[1] == 0 && obj->s[2] == 0 && obj->s[3] == 0) {
        obj->s[0] = 0x12345678;
    }
    return obj;
}


MAKE_UINT64_PRNG("xoshiro256++:mod", NULL)
