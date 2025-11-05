/**
 * @file ranrot32.c
 * @brief Implementation of RANROT32 generator: a modified lagged Fibonacci
 * pseudorandom number generator.
 * @details The RANROT generators were suggested by Agner Fog. They
 * resemble additive lagged Fibonacci generators but use extra rotations
 * to bypass such tests as birthday spacings, gap test etc. However, the
 * underlying theory is not studied very well and minimal period is unknown!
 *
 * RANROT32 passes `bspace`, `gap` and `gap16` tests but fails `dc6_long` test
 * based on Hamming weights of 256-bit words.
 *
 * The PRNG parameters are taken from PractRand source code.
 *
 * WARNING! THE MINIMAL PERIOD OF RANROT IS UNKNOWN! It was added mainly for
 * testing the `dc6_long` test and shouldn't be used in practice!
 *
 * References:
 *
 *  1. Agner Fog. Chaotic Random Number Generators with Random Cycle Lengths.
 *     2001. https://www.agner.org/random/theory/chaosran.pdf
 *  2. https://www.agner.org/random/discuss/read.php?i=138#138
 *  3. https://pracrand.sourceforge.net/
 *
 * @copyright RANROT algorithm was developed by Agner Fog, the used parameters
 * were taken from PractRand 0.94 by Chris Doty-Humphrey.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define ROT1 9
#define ROT2 13

PRNG_CMODULE_PROLOG

typedef struct {
    int pos;
    int lag1;
    int lag2;
    uint32_t *x;
} RanRot32State;

/**
 * @brief An optimized implementation of lagged Fibonacci style PRNG.
 * @details The buffer before refilling looks like:
 *
 *     [x_{-r} x_{-(r-1)} ... x_{-1}]
 *
 * The first cycle works goes until the x_{n-s} will hit the right
 * boundary. The second cycle processes the rest of the array and
 * works in its opposite sides.
 */
static inline uint64_t get_bits_raw(void *state)
{
    RanRot32State *obj = state;
    uint32_t *x = obj->x;
    if (obj->pos != 0) {
        return x[--obj->pos];
    }
    int dlag = obj->lag1 - obj->lag2;
    for (int i = 0; i < obj->lag2; i++) {
        x[i] = rotl32(x[i], ROT1) + rotl32(x[i + dlag], ROT2);
    }
    for (int i = obj->lag2; i < obj->lag1; i++) {
        x[i] = rotl32(x[i], ROT1) + rotl32(x[i - obj->lag2], ROT2);
    }
    obj->pos = obj->lag1;
    return x[--obj->pos];
}


static void *create(const CallerAPI *intf)
{
    int lag1 = 17, lag2 = 9;
    const char *param = intf->get_param();
    if (!intf->strcmp("7_3", param)) {
        lag1 = 7; lag2 = 3;
    } else if (!intf->strcmp("17_9", param) || !intf->strcmp("", param)) {
        lag1 = 17; lag2 = 9;
    } else if (!intf->strcmp("57_13", param)) {
        lag1 = 57; lag2 = 13;
    } else {
        intf->printf("Unknown parameter %s\n", param);
        return NULL;
    }
    intf->printf("RANROT32(%d,%d)\n", lag1, lag2);
    RanRot32State *obj = intf->malloc(sizeof(RanRot32State) + (unsigned int) lag1 * sizeof(uint32_t));   
    obj->x = (uint32_t *) (void *) ( (char *) obj + sizeof(RanRot32State) );
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    obj->lag1 = lag1; obj->lag2 = lag2;
    for (int i = 0; i < obj->lag2; i++) {
        obj->x[i] = (uint32_t) (pcg_bits64(&state) >> 32);
    }
    obj->pos = 0; // Mark buffer uninitialized
    return (void *) obj;
}


MAKE_UINT32_PRNG("ranrot32", NULL)
