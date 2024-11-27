/**
 * @file lcg96_portable_shared.c
 * @brief Just 96-bit LCG with \f$ m = 2^{96}\f$ that is written without
 * usage of C extensions. Can be compiled by GCC for 32-bit platforms
 * such as x86 before AMD64. Requires `uint64_t`/`unsigned long long`
 * types introduced in C99.
 *
 * @details The multipliers can be taken from:
 *
 * 1. P. L'Ecuyer. Tables of linear congruential generators of different
 *    sizes and good lattice structure // Mathematics of Computation. 1999.
 *    V. 68. N. 225. P. 249-260
 *    http://dx.doi.org/10.1090/S0025-5718-99-00996-5
 * 2. https://www.pcg-random.org/posts/does-it-beat-the-minimal-standard.html
 *
 * The multiplier from [1] is used. However, both variants fail 32-bit
 * 8-dimensional decimated birthday spacings test `bspace4_8d_dec`. They also
 * fail TMFn test from PractRand 0.94.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 96-bit portable LCG state.
 * @details Not optimized for byte order (low endian/big endian) of any
 * specific platform. But `x[0]` is the lower 32-bit word and `x[2]` is
 * the higher 32-bit word.
 */
typedef struct {
    uint32_t x[3];
} Lcg96x32State;


#define HI64(x) ((x) >> 32)
#define LO64(x) ((x) & 0xFFFFFFFF)
#define MUL64(x,y) ((uint64_t)(x) * (uint64_t)(y))
#define SUM64(x,y) ((uint64_t)(x) + (uint64_t)(y))

/**
 * @brief A portable implementation of 96-bit LCG.
 */
static inline uint64_t get_bits_raw(void *state)
{
    //                           lower        medium     high
    static const uint32_t a[] = {0x3bda4a15, 0xfa75832c, 0xf429e3c0};
    static const uint32_t c = 1;
    uint32_t row0[3], row1[2], row2;
    uint64_t mul, sum;
    Lcg96x32State *obj = state;
    // Row 0
    mul = MUL64(a[0], obj->x[0]); row0[0] = LO64(mul);
    mul = MUL64(a[0], obj->x[1]) + HI64(mul); row0[1] = LO64(mul);
    mul = MUL64(a[0], obj->x[2]) + HI64(mul); row0[2] = LO64(mul);
    // Row 1
    mul = MUL64(a[1], obj->x[0]); row1[0] = LO64(mul);
    mul = MUL64(a[1], obj->x[1]) + HI64(mul); row1[1] = LO64(mul);
    // Row 2
    row2 = LO64(MUL64(a[2], obj->x[0]));
    // Sum rows (update state)
    sum = SUM64(row0[0], c);                   obj->x[0] = LO64(sum);
    sum = SUM64(row0[1], row1[0]) + HI64(sum); obj->x[1] = LO64(sum);
    sum = SUM64(row0[2], row1[1]) + row2 + HI64(sum);
    obj->x[2] = LO64(sum);
    // Return upper 32 bits
    return obj->x[2];
}


static void *create(const CallerAPI *intf)
{
    Lcg96x32State *obj = intf->malloc(sizeof(Lcg96x32State));
    uint64_t seed = intf->get_seed64();
    obj->x[0] = LO64(seed) | 0x1; // For the case of MCG
    obj->x[1] = HI64(seed);
    obj->x[2] = 0;
    return (void *) obj;
}

/**
 * @brief Self-test to check portable implementation of 96-bit
 * multiplication and addition.
 * @details It is easy to reproduce reference values in Python:
 *
 *     a = 0xf429e3c0fa75832c3bda4a15
 *     x = 1234567890
 *     for i in range(0,1000000):
 *         x = (a*x + 1) % 2**96
 *     print(hex(x))
 *
 */
static int run_self_test(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x6a5efd72;
    Lcg96x32State obj;
    obj.x[0] = 1234567890; obj.x[1] = 0; obj.x[2] = 0;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Lcg96x32", run_self_test)
