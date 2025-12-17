//
/*

TestU01: BigCrush
SmokeRand: full
64-bit birthday paradox test: passes

rng=RNG_stdin32, seed=unknown
length= 2 terabytes (2^41 bytes), time= 7651 seconds
  no anomalies in 313 test result(s)

rng=RNG_stdin32, seed=unknown
length= 4 terabytes (2^42 bytes), time= 14449 seconds
  Test Name                         Raw       Processed     Evaluation
  Gap-16:A                          R=  -4.8  p =1-5.9e-4   unusual
  ...and 322 test result(s) without anomalies

rng=RNG_stdin32, seed=unknown
length= 8 terabytes (2^43 bytes), time= 27521 seconds
  Test Name                         Raw       Processed     Evaluation
  FPF-14+6/16:all                   R=  -7.5  p =1-7.3e-7   suspicious
  ...and 330 test result(s) without anomalies
*/
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief Komirand16-Weyl PRNG state.
 */
typedef struct {
    uint16_t st1;
    uint16_t st2;
    uint16_t w;
} Komirand16WeylState;

static inline uint16_t get_bits16(Komirand16WeylState *state)
{
    //static const uint16_t inc = 0x9E37;
    uint16_t s1 = state->st1, s2 = state->st2;
    s2 += (uint16_t) state->w;
    s1 += (uint16_t) ( rotl16(s2, 3) ^ rotl16(s2, 8) ^ s2);
    s2 ^= (uint16_t) ( rotl16(s1, 15) + rotl16(s1, 8) + s1 );
    //s1 ^= (s1 >> 8);
    //s2 ^= (s2 >> 8);
    state->st1 = s2;
    state->st2 = s1;
    state->w++;//= inc;
    return (state->st1 ^ state->st2);
}

static inline uint64_t get_bits_raw(Komirand16WeylState *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Komirand16WeylState *obj = intf->malloc(sizeof(Komirand16WeylState));
    uint64_t seed = intf->get_seed64();
    obj->st1 = (uint16_t) seed;
    obj->st2 = (uint16_t) (seed >> 16);
    obj->w   = (uint16_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("a16Weyl", NULL)
