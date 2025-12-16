#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief Komirand16-Weyl PRNG state.
 */
typedef struct {
    uint64_t st1;
    uint64_t st2;
    uint64_t w;
} Komirand16WeylState;

static inline uint64_t get_bits_raw(Komirand16WeylState *state)
{
    //static const uint16_t inc = 0x9E37;
    uint64_t s1 = state->st1, s2 = state->st2;
    const uint64_t out = s1 ^ s2;
    s2 += state->w;
    s1 += ( rotl64(s2, 7) ^ rotl64(s2, 32) ^ s2);
    s2 ^= ( rotl64(s1, 63) + rotl64(s1, 32) + s1 );
    state->st1 = s2;
    state->st2 = s1;
    state->w++;//= inc;
    return out;
}

static void *create(const CallerAPI *intf)
{
    Komirand16WeylState *obj = intf->malloc(sizeof(Komirand16WeylState));
    obj->st1 = intf->get_seed64();
    obj->st2 = intf->get_seed64();
    obj->w   = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("a64Weyl", NULL)
