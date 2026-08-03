/**
 * @file xorrot256.c
 * @brief A xorshift-style LFSR that has the \f$2^{256} - 1\f$ period.
 * @details It uses the next step matrix:
 *
 *                  | I 0 I A |
 *     |x y z w | * | I 0 0 0 |
 *                  | 0 I 0 0 |
 *                  | 0 0 I B |
 *
 * Known triples:
 *
 * - (3, 8, 37) - passes `hamming.cfg` and >= 8 Tib PractRand 0.96
 *                (except BRank test for binary matrices ranks)
 * - (5, 12, 35) - passes `hamming.cfg`.
 *
 * - (1, 1, 35) - doesn't give a full period but A^period = A
 *
 * The algorithm is designed by A.L. Voskov.
 *
 * References:
 *
 * 1. Ronald L. Rivest. On the invertibility of the XOR of rotations of
 *    a binary word https://people.csail.mit.edu/rivest/pubs/Riv11e.prepub.pdf
 * 2. Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *    V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 * 3. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
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
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint64_t w;
} Xorrot256State;


///////////////////////////
///// Default version /////
///////////////////////////

/**
 * @brief Its period is \f$ 2^{256} - 1 \f$.
 */
static inline uint64_t get_bits_good_raw(Xorrot256State *obj)
{
    const uint64_t x0 = obj->x, w0 = obj->w;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 3) ^ obj->z ^ rotl64(w0, 8) ^ rotl64(w0, 37);
    return x0;
}

MAKE_GET_BITS_WRAPPERS(good)

///////////////////////
///// Test case 1 /////
///////////////////////

/**
 * @brief Its period is not \f$ 2^{256} - 1 \f$. Introduced as
 * a test case for the `lfsr` battery.
 */
static inline uint64_t get_bits_bad1_raw(Xorrot256State *obj)
{
    const uint64_t x0 = obj->x, w0 = obj->w;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 3) ^ obj->z ^ rotl64(w0, 8) ^ rotl64(w0, 39);
    return x0;
}

MAKE_GET_BITS_WRAPPERS(bad1)


///////////////////////
///// Test case 2 /////
///////////////////////

/**
 * @brief Its period is not \f$ 2^{256} - 1 \f$ but A^{2**256} = I.
 * So it is a good non-trivial test case for the `lfsr` battery.
 */
static inline uint64_t get_bits_bad2_raw(Xorrot256State *obj)
{
    const uint64_t x0 = obj->x, w0 = obj->w;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 1) ^ obj->z ^ rotl64(w0, 1) ^ rotl64(w0, 35);
    return x0;
    return obj->x;
}

MAKE_GET_BITS_WRAPPERS(bad2)


static void *create(const CallerAPI *intf)
{
    Xorrot256State *obj = intf->malloc(sizeof(Xorrot256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    if (obj->w == 0) {
        obj->w = 0x123456789ABCDEF;
    }
    return obj;
}

/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t
        x_ref = 0x5e297db916d40034, y_ref=0xcedb4a60b92555bd,
        z_ref = 0x68d87952d71e7344, w_ref=0x7900cabfb34f48cc;
    Xorrot256State obj = {
        .x = 0x123456789ABCDEF, .y = 0xFEDCBA987654321,
        .z = 0xDEADBEEF,        .w = 0xBADF00D
    };
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_good_raw(&obj);
    }
    intf->printf("x_out = %llX; x_ref = %llX\n",
        (unsigned long long) obj.x, (unsigned long long) x_ref);
    intf->printf("y_out = %llX; y_ref = %llX\n",
        (unsigned long long) obj.y, (unsigned long long) y_ref);
    intf->printf("z_out = %llX; z_ref = %llX\n",
        (unsigned long long) obj.z, (unsigned long long) z_ref);
    intf->printf("w_out = %llX; w_ref = %llX\n",
        (unsigned long long) obj.w, (unsigned long long) w_ref);
    return (obj.x == x_ref && obj.y == y_ref &&
            obj.z == z_ref && obj.w == w_ref);
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "xorrot256", 64, default_create, get_bits_good, get_sum_good},
    {"default",   "xorrot256", 64, default_create, get_bits_good, get_sum_good},
    {"bad1",      "xorrot256:bad1", 64, default_create, get_bits_bad1, get_sum_bad1},
    {"bad2",      "xorrot256:bad2", 64, default_create, get_bits_bad2, get_sum_bad2},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The xorrot256 PRNG is a xorshift-style LFSR with a maximal period.\n"
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
