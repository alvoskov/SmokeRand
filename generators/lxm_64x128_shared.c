/**
 * @file lxm_64x128_shared.c
 * @brief Combined LXM algorithm, the L64X128MixRandom modification.
 * @details The algorithm resembles combined SuperDuper generators by
 * G.Marsaglia et al. It uses 64-bit LGG with \f$ m=2^{64} \f$ together with
 * xoroshiro128 LFSR. It also contains the LEA64 output function (suggested
 * by Doug Lea) to improve PRNG quality, i.e. hide artefacts typical for LCG
 * and LFSR.
 *
 * References:
 *
 * 1. Guy L. Steele Jr., Sebastiano Vigna. LXM: better splittable pseudorandom
 *    number generators (and almost as fast). Proc. ACM Program. Lang. 5, OOPSLA,
 *    Article 148 (October 2021), 31 pages. http://doi.org/10.1145/3485525
 * 2. https://github.com/openjdk/jdk/blob/master/src/java.base/share/classes/jdk/internal/random/L64X128MixRandom.java
 *
 * @copyright LXM algorithm was developed by Guy L. Steele Jr. and Sebastiano Vigna
 *
 * C implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LXM (L64X128MixRandom) PRNG state.
 */
typedef struct {
    uint64_t lcg; ///< LCG state
    uint64_t x0;  ///< LFSR state
    uint64_t x1;  ///< LFSR state
} LXMState;

static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t a = 0xd1342543de82ef95ull; // LCG multiplier
    static const uint64_t c = 12345; // LCG constant
    static const uint64_t lea64_m = 0xdaba0b6eb09322e3ull; // LEA64 multiplier
    LXMState *obj = state;
    // Create output from PRNG state using LEA64 mixing function
    uint64_t z = obj->lcg + obj->x0;
    // Mixing function (lea64)
    z = (z ^ (z >> 32)) * lea64_m;
    z = (z ^ (z >> 32)) * lea64_m;
    z = (z ^ (z >> 32));
    // Update the LCG subgenerator
    obj->lcg = a * obj->lcg + c;
    // Update the XBG subgenerator (xoroshiro128v1_0)
    uint64_t x0 = obj->x0, x1 = obj->x1;
    x1 ^= x0;
    x0 = rotl64(x0, 64) ^ x1 ^ (x1 << 16);
    x1 = rotl64(x1, 37);
    obj->x0 = x0; obj->x1 = x1;
    // Return result
    return z;
}


static void *create(const CallerAPI *intf)
{
    LXMState *obj = intf->malloc(sizeof(LXMState));
    obj->lcg = intf->get_seed64();
    obj->x0 = intf->get_seed64();
    obj->x1 = intf->get_seed64();
    if (obj->x0 == 0 && obj->x1 == 0) {
        obj->x1 = 0xDEADBEEFDEADBEEF;
    }
    return (void *) obj;
}

MAKE_UINT64_PRNG("LXM(L64X128MixRandom)", NULL)
