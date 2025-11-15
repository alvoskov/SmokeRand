/**
 * @file ranrot32.c
 * @brief Implementation of RANROT32 generator: a modified lagged Fibonacci
 * pseudorandom number generator.
 * @details The RANROT generators were suggested by Agner Fog. They
 * resemble additive lagged Fibonacci generators but use extra rotations
 * to bypass such tests as birthday spacings, gap test etc. However, the
 * underlying theory is not studied very well and minimal period is unknown!
 *
 * RANROT32 passes `bspace`, `gap` and `gap16` tests but fails the Hamming
 * weights based tests like `hamming_ot_u128` or `hamming_ot_u256`.
 *
 * The PRNG parameters are taken from PractRand source code.
 *
 * WARNING! THE MINIMAL PERIOD OF RANROT IS UNKNOWN! It was added mainly for
 * testing the `hamming_ot` tests and shouldn't be used in practice!
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
static inline uint64_t get_bits_ranrot32_raw(void *state)
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

MAKE_GET_BITS_WRAPPERS(ranrot32)

static void *create_ranrot32(const CallerAPI *intf, int lag1, int lag2)
{
    RanRot32State *obj = intf->malloc(sizeof(RanRot32State) + (unsigned int) lag1 * sizeof(uint32_t));   
    obj->x = (uint32_t *) (void *) ( (char *) obj + sizeof(RanRot32State) );
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    obj->lag1 = lag1; obj->lag2 = lag2;
    for (int i = 0; i < obj->lag2; i++) {
        obj->x[i] = (uint32_t) (pcg_bits64(&state) >> 32);
    }
    obj->pos = 0; // Mark buffer uninitialized
    return obj;
}

static void *create_7_3(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_ranrot32(intf, 7, 3);
}

static void *create_17_9(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_ranrot32(intf, 17, 9);
}

static void *create_57_13(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_ranrot32(intf, 57, 13);
}

static void *create(const CallerAPI *intf)
{
    intf->printf("'%s' not implemented\n", intf->get_param());
    return NULL;
}

static const char description[] =
"RANROT32 generator: a nonlinear modification of additive lagged Fibonacci\n"
"generator suggested by Agner Fog. Its minimal period is unknown and it is\n"
"an experimental generator that shouldn't be used in production\n"
"The next param values are supported:\n"
"  7_3   - RANROT(7,  3,  2^32, sh1=9, sh2=13)\n"
" 17_9   - RANROT(17, 9,  2^32, sh1=9, sh2=13) - the default one\n"
" 57_13  - RANROT(57, 13, 2^32, sh1=9, sh2=13)\n";


static const GeneratorParamVariant gen_list[] = {
    {"7_3",   "ranrot32:7_3",   32, create_7_3,   get_bits_ranrot32, get_sum_ranrot32},
    {"",      "ranrot32:17_9",  32, create_17_9,  get_bits_ranrot32, get_sum_ranrot32},
    {"17_9",  "ranrot32:17_9",  32, create_17_9,  get_bits_ranrot32, get_sum_ranrot32},
    {"57_13", "ranrot32:57_13", 32, create_57_13, get_bits_ranrot32, get_sum_ranrot32},
    {NULL, NULL, 32, NULL, NULL, NULL}
};


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
