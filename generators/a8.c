// PractRand: 2 MiB
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief Komirand16-Weyl PRNG state.
 */
typedef struct {
    uint8_t st1;
    uint8_t st2;
    uint8_t w;
} Komirand16WeylState;

static inline uint8_t get_bits8(Komirand16WeylState *state)
{
    static const uint8_t inc = 0x9D;
    uint8_t s1 = state->st1, s2 = state->st2;
    s2 += state->w;
    s1 += (uint8_t) ( rotl8(s2, 1) ^ rotl8(s2, 4) ^ s2);
    s2 ^= (uint8_t) ( rotl8(s1, 7) + rotl8(s1, 4) + s1 );
    state->st1 = s2;
    state->st2 = s1;
    state->w += inc;
    //state->w++;
    return (state->st1 ^ state->st2);
}

static inline uint64_t get_bits_raw(Komirand16WeylState *state)
{
    const uint32_t a = get_bits8(state);
    const uint32_t b = get_bits8(state);
    const uint32_t c = get_bits8(state);
    const uint32_t d = get_bits8(state);
    return a | (b << 8) | (c << 16) | (d << 24);
}


static void *create(const CallerAPI *intf)
{
    Komirand16WeylState *obj = intf->malloc(sizeof(Komirand16WeylState));
    uint64_t seed = intf->get_seed64();
    obj->st1 = (uint8_t) seed;
    obj->st2 = (uint8_t) (seed >> 16);
    obj->w   = (uint8_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("a8Weyl", NULL)
