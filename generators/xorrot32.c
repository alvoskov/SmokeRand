/**
 * @file xorrot32.c
 * @brief xorrot32 is a LFSR with 32-bit state, its period is \f$2^{32} - 1\f$.
 * @details The algorithm is suggested by A. L. Voskov. It uses a reversible
 * operation based on XORs of odd numbers of rotations from [1].
 *
 * References:
 *
 * 1. Ronald L. Rivest. On the invertibility of the XOR of rotations of
 *    a binary word https://people.csail.mit.edu/rivest/pubs/Riv11e.prepub.pdf
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x;
} Xorrot32State;


///////////////////////////
///// Default version /////
///////////////////////////

/**
 * @brief Its period is \f$ 2^{32} - 1 \f$. May be used in some
 * combined generators or as a fairly good toy generator.
 */
static inline uint64_t get_bits_good_raw(Xorrot32State *obj)
{
    obj->x ^= obj->x << 1;
    obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
    return obj->x;
}

MAKE_GET_BITS_WRAPPERS(good)

///////////////////////
///// Test case 1 /////
///////////////////////

/**
 * @brief Its period is not \f$ 2^{32} - 1 \f$. Introduced as
 * a test case for the `lfsr` battery.
 */
static inline uint64_t get_bits_bad1_raw(Xorrot32State *obj)
{
    obj->x ^= obj->x << 2;
    obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
    return obj->x;
}

MAKE_GET_BITS_WRAPPERS(bad1)


///////////////////////
///// Test case 2 /////
///////////////////////

/**
 * @brief Its period is not \f$ 2^{32} - 1 \f$ but A^{2**32} = I.
 * So it is a good non-trivial test case for the `lfsr` battery.
 */
static inline uint64_t get_bits_bad2_raw(Xorrot32State *obj)
{
    obj->x ^= obj->x << 1;
    obj->x ^= rotl32(obj->x, 6) ^ rotl32(obj->x, 23);
    return obj->x;
}

MAKE_GET_BITS_WRAPPERS(bad2)




static void *create(const CallerAPI *intf)
{
    Xorrot32State *obj = intf->malloc(sizeof(Xorrot32State));
    obj->x = intf->get_seed32();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    const uint32_t u_ref = 0x40C52DA7;
    uint32_t u;
    Xorrot32State obj = {.x = 0x12345678};
    for (long i = 0; i < 10000000; i++) {
        u = (uint32_t) get_bits_good_raw(&obj);
    }
    intf->printf("u = %lX; u_ref = %lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}

static const GeneratorParamVariant gen_list[] = {
    {"",          "xorrot32", 32, default_create, get_bits_good, get_sum_good},
    {"default",   "xorrot32", 32, default_create, get_bits_good, get_sum_good},
    {"bad1",      "xorrot32", 32, default_create, get_bits_bad1, get_sum_bad1},
    {"bad2",      "xorrot32", 32, default_create, get_bits_bad2, get_sum_bad2},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The xorrot32 PRNG is a xorshift-style LFSR with a maximal period.\n"
"The next param values are supported:\n"
"  default   - good version\n"
"  bad1      - a version with a reduced period (test case 1)\n"
"  bad2      - a version with a reduced period (test case 2)\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
