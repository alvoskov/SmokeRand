/**
 * @file minstd.c
 * @brief Obsolete "minimal standard" 31-bit LCG with prime modulus.
 * It is \f$ LCG(2^{31} - 1, 16807, 0) \f$.
 *
 * @detials It is an obsolete generator that fails SmallCrush, Crush, BigCrush
 * and PractRand and its speed is comparable to SIMD version of ChaCha12 on
 * modern 64-bit x86-64 processors. This implementation of minstd includes
 * two versions:
 *
 * - `mul32` that is based on "Integer version 2" from [1]. It uses Schrage's
 *   method to be able to use only 32-bit arithmetics.
 * - `mul64` that relies on 32-bit x 32-bit multiplication that returns the
 *   upper 32 bits of 64-bit product.
 *
 * The `mul64` version is usually faster on 32-bit and 64-bit processors:
 * the required multiplication instruction is available on x86 architecture
 * since 80386.
 *
 * This version of minstd uses the output scrambler that extends its 31-bit
 * output to 32 bits by the `(x << 1) | (x >> 30)` transformation.
 *
 * Reference:
 *
 * 1. S. K. Park, K. W. Miller. Random number generators: good ones
 *    are hard to find // Communications of the ACM. 1988. V. 31. N 10.
 *    P.1192-1201. https://doi.org/10.1145/63039.63042
 * 2. https://programmingpraxis.com/2014/01/14/minimum-standard-random-number-generator/
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief minstd "Integer version 2" based on Schrage's method.
 */
static inline uint64_t get_bits_mul32_raw(void *state)
{
    static const int32_t m = 2147483647, a = 16807;
    static const int32_t q = 127773, r = 2836; // (m div a) and (m mod a)
    Lcg32State *obj = state;
    const int32_t x = (int32_t) obj->x;
    const int32_t hi = x / q;
    const int32_t lo = x - q * hi; // It is x mod q
    const int32_t t = a * lo - r * hi;
    obj->x = (uint32_t) ((t < 0) ? (t + m) : t);
    return (obj->x << 1) | (obj->x >> 30);
}

MAKE_GET_BITS_WRAPPERS(mul32)

/**
 * @brief minstd "Real version 1" based on 64-bit multiplication with
 * two 32-bit input arguments.
 */
static inline uint64_t get_bits_mul64_raw(void *state)
{
    Lcg32State *obj = state;
    uint64_t prod = 16807ULL * obj->x;
    uint32_t q = (uint32_t) (prod & 0x7FFFFFFFU);
    uint32_t p = (uint32_t) (prod >> 31);
    obj->x = p + q;
    if (obj->x >= 0x7FFFFFFFU) {
        obj->x -= 0x7FFFFFFFU;
    }
    return (obj->x << 1) | (obj->x >> 30);
}


MAKE_GET_BITS_WRAPPERS(mul64)


static void *create(const CallerAPI *intf)
{
    Lcg32State *obj = intf->malloc(sizeof(Lcg32State));
    obj->x = (uint32_t) (intf->get_seed64() >> 33);
    return (void *) obj;
}


int run_self_test(const CallerAPI *intf)
{
    const uint32_t x_ref = 1043618065;
    Lcg32State obj;
    obj.x = 1;
    for (size_t i = 0; i < 10000; i++) {
        get_bits_mul32_raw(&obj.x);
    }
    int is_ok = 1;
    intf->printf("Mul32 version testing results\n");
    intf->printf("The current state is %d, reference value is %d\n",
        (int) obj.x, (int) x_ref);
    if (obj.x != x_ref) is_ok = 0;

    obj.x = 1;
    for (size_t i = 0; i < 10000; i++) {
        get_bits_mul32_raw(&obj.x);
    }

    intf->printf("Mul64 version testing results\n");
    intf->printf("The current state is %d, reference value is %d\n",
        (int) obj.x, (int) x_ref);
    if (obj.x != x_ref) is_ok = 0;

    return is_ok;
}


static const char description[] =
"minstd: a classic but obsolete 'minimal standard' LCG.\n"
"  mul32 - version with 32-bit multiplication (Schrage's method).\n"
"  mul64 - version with 64-bit multiplication (default).\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 32;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "mul64") || !intf->strcmp(param, "")) {
        gi->name = "minstd:mul64";
        gi->get_bits = get_bits_mul64;
        gi->get_sum = get_sum_mul64;
    } else if (!intf->strcmp(param, "mul32")) {
        gi->name = "minstd:mul32";
        gi->get_bits = get_bits_mul32;
        gi->get_sum = get_sum_mul32;
    } else {
        gi->name = "minstd:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
