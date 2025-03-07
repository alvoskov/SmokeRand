/**
 * @file lcg128_u32_portable.c
 * @brief Just 128-bit LCG with \f$ m = 2^{128}\f$ that is written without
 * usage of C extensions. Can be compiled by GCC for 32-bit platforms
 * such as x86 before AMD64. Requires `uint64_t`/`unsigned long long`
 * types introduced in C99.
 *
 * @details The multipliers can be taken from:
 *
 * 1. https://doi.org/10.1002/spe.3030 doesn't improve PractRand 0.94 results.

 *
 * This PRNG fails 32-bit 8-dimensional decimated birthday spacings test
 * `bspace4_8d_dec` but passes TMFn test from PractRand 0.94 at 32 TiB samples.
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
 * @brief 128-bit portable LCG state.
 * @details Not optimized for byte order (low endian/big endian) of any
 * specific platform. But `x[0]` is the lower 32-bit word and `x[3]` is
 * the higher 32-bit word.
 */
typedef struct {
    uint32_t x[4];
} Lcg128x32State;


#define HI64(x) ((x) >> 32)
#define LO64(x) ((x) & 0xFFFFFFFF)
#define MUL64(x,y) ((uint64_t)(x) * (uint64_t)(y))
#define SUM64(x,y) ((uint64_t)(x) + (uint64_t)(y))

/**
 * @brief A portable implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_raw(void *state)
{
    //                           lower        higher
    static const uint32_t a[] = {0x0b15f4fd, 0xfc0072fa};
    static const uint32_t c = 12345;
    uint32_t row0[4], row1[3];
    uint64_t mul, sum;
    Lcg128x32State *obj = state;
    // Row 0
    mul = MUL64(a[0], obj->x[0]); row0[0] = LO64(mul);
    mul = MUL64(a[0], obj->x[1]) + HI64(mul); row0[1] = LO64(mul);
    mul = MUL64(a[0], obj->x[2]) + HI64(mul); row0[2] = LO64(mul);
    mul = MUL64(a[0], obj->x[3]) + HI64(mul); row0[3] = LO64(mul);
    // Row 1
    mul = MUL64(a[1], obj->x[0]); row1[0] = LO64(mul);
    mul = MUL64(a[1], obj->x[1]) + HI64(mul); row1[1] = LO64(mul);
    mul = MUL64(a[1], obj->x[2]) + HI64(mul); row1[2] = LO64(mul);
    // Sum rows (update state)
    sum = SUM64(row0[0], c);                   obj->x[0] = LO64(sum);
    sum = SUM64(row0[1], row1[0]) + HI64(sum); obj->x[1] = LO64(sum);
    sum = SUM64(row0[2], row1[1]) + HI64(sum); obj->x[2] = LO64(sum);
    sum = SUM64(row0[3], row1[2]) + HI64(sum); obj->x[3] = LO64(sum);
    // Return upper 32 bits
    return obj->x[3];
}


static void *create(const CallerAPI *intf)
{
    Lcg128x32State *obj = intf->malloc(sizeof(Lcg128x32State));
    uint64_t seed0 = intf->get_seed64();
    uint64_t seed1 = intf->get_seed64();
    obj->x[0] = LO64(seed0) | 0x1; // For the case of MCG
    obj->x[1] = HI64(seed0);
    obj->x[2] = LO64(seed1);
    obj->x[3] = HI64(seed1);
    return (void *) obj;
}

/**
 * @brief Self-test to check portable implementation of 96-bit
 * multiplication and addition.
 * @details It is easy to reproduce reference values in Python:
 *
 *     a = 0xfc0072fa0b15f4fd
 *     x = 1234567890
 *     for i in range(0,1000000):
 *         x = (a*x + 12345) % 2**128
 *     print(hex(x))
 *
 */
static int run_self_test(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x63ea2cac;
    Lcg128x32State obj;
    obj.x[0] = 1234567890; obj.x[1] = 0; obj.x[2] = 0; obj.x[3] = 0;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Lcg128x32", run_self_test)
