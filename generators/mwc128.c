/**
 * @file mwc128.c
 * @brief MWC128 - 128-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^127.
 * Passes SmallCrush, Crush and BigCrush tests.
 * The MWC_A1 multiplier was found by S. Vigna.
 *
 * References:
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
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
 * @brief MWC128 state. Cannot be initialized to (0, 0) or to
 * (2^64 - 1, 2^64 - 1). Default initialization is (seed, 1) as suggested
 * by S. Vigna.
 */
typedef struct {
    uint64_t x;
    uint64_t c;
} MWC128State;


#define HI64(x) ((x) >> 32)
#define LO64(x) ((x) & 0xFFFFFFFF)
#define MUL64(x,y) ((uint64_t)(x) * (uint64_t)(y))
#define SUM64(x,y) ((uint64_t)(x) + (uint64_t)(y))


static inline void mul_add_64x64(uint64_t a, uint32_t *x, uint64_t c)
{
    uint32_t row0[3], row1[3];
    uint64_t mul, sum;
    uint32_t a_lo = LO64(a), a_hi = HI64(a);
    // Row 0
    mul = MUL64(a_lo, x[0]); row0[0] = LO64(mul);
    mul = MUL64(a_lo, x[1]) + HI64(mul); row0[1] = LO64(mul);
    mul = MUL64(a_lo, x[2]) + HI64(mul); row0[2] = LO64(mul);
    // Row 1
    mul = MUL64(a_hi, x[0]); row1[0] = LO64(mul);
    mul = MUL64(a_hi, x[1]) + HI64(mul); row1[1] = LO64(mul);
    mul = MUL64(a_hi, x[2]) + HI64(mul); row1[2] = LO64(mul);
    // Sum rows (update state)
    sum = SUM64(row0[0], LO64(c));                       x[0] = LO64(sum);
    sum = SUM64(row0[1], row1[0]) + HI64(c) + HI64(sum); x[1] = LO64(sum);
    sum = SUM64(row0[2], row1[1]) + HI64(sum);           x[2] = LO64(sum);
    sum = SUM64(HI64(sum), row1[2]);                     x[3] = LO64(sum);
}


/**
 * @brief MWC128 PRNG implementation.
 */
static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t MWC_A1 = 0xffebb71d94fcdaf9;
    MWC128State *obj = state;
#ifndef UMUL128_FUNC_ENABLED
    uint32_t x[4];
    x[0] = LO64(obj->x); x[1] = HI64(obj->x); x[2] = 0; x[3] = 0;
    mul_add_64x64(MWC_A1, x, c);
    obj->x = ((uint64_t) x[0]) | (((uint64_t) x[1]) << 32);    
    obj->c = ((uint64_t) x[2]) | (((uint64_t) x[3]) << 32);    
#elseif defined(UINT128_ENABLED)
    const __uint128_t t = MWC_A1 * (__uint128_t)obj->x + obj->c;
    obj->x = t;
    obj->c = t >> 64;
#else
    uint64_t c_old = obj->c;
    obj->x = unsigned_mul128(MWC_A1, obj->x, &obj->c);
    obj->c += _addcarry_u64(0, obj->x, c_old, &obj->x);
#endif
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    MWC128State *obj = intf->malloc(sizeof(MWC128State));
    obj->x = intf->get_seed64();
    obj->c = 1;
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    MWC128State obj = {.x = 12345, .c = 67890};
    uint64_t u, u_ref = 0x72BD413ED8304C94;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("MWC128", run_self_test)
