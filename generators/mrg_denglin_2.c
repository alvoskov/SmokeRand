/**
 * @file mrg_denglin_2.c
 * @brief A variant of multiple recursive generator with 31-bit output,
 * period around `2**62`. Fails a lot of statistical tests.
 * @details It is based on the next recurrent formula:
 *
 * \f[
 * x_i = 46338 x_{i-2} - x_{i-1} \mod \left(2^{31} - 1\right)
 * \f]
 *
 * References:
 *
 * 1. Deng L., Lin D. Random Number Generation for the New Century // American
 *    Statistician. 2000. V. 54. N.2. P. 145-150.
 *    https://doi.org/10.1080/00031305.2000.10474528
 *
 * The script for the period verification:
 *
 *    import galois
 *    GFp = galois.GF(2**31 - 1)
 *    poly_to_test = galois.Poly([1, 1, -46338], field=GFp)
 *    is_primitive = poly_to_test.is_primitive()
 *    print(f"Is {poly_to_test} primitive? {is_primitive}")
 *
 * @copyright This MRG modification was developed by Deng L. and Lin D.
 * 
 * Reentrant C99 implementation with internal self-tests:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x[2];
} MrgDengLin2State;


static inline uint32_t mod_mp31(uint64_t x)
{
    const uint32_t q = (uint32_t) (x & 0x7FFFFFFF);
    const uint32_t p = (uint32_t) (x >> 31);
    x = p + q;
    if (x >= 0x7FFFFFFF) {
        x -= 0x7FFFFFFF;
    }
    return (uint32_t) x;
}


static inline uint64_t get_bits_raw(MrgDengLin2State *obj)
{
    int64_t prod = 46338 * (int64_t) obj->x[1] - (int64_t) obj->x[0];
    if (prod < 0) {
        prod += 0x7FFFFFFF;
    }
    obj->x[1] = obj->x[0];
    obj->x[0] = mod_mp31((uint64_t) prod);
    return obj->x[0] << 1;
}


static void *create(const CallerAPI *intf)
{
    MrgDengLin2State *obj = intf->malloc(sizeof(MrgDengLin2State));
    seed64_to_2x32(intf, &obj->x[0], &obj->x[1]);
    for (int i = 0; i < 2; i++) {
        obj->x[i] &= 0x7FFFFFFF;
        if (obj->x[i] == 0 || obj->x[i] == 0x7FFFFFFF) {
            obj->x[i] = 1;
        }
    }
    return obj;
}

/**
 * @brief The test is based on the values obtained from Python script.
 * @details The used script:
 *
 *    x = [1, 2]
 *    for i in range(0, 1_000_000):
 *        x_new = (46338*x[1] - x[0]) % (2**31 - 1)
 *        x = [x_new, x[0]]
 *    print(x[0] << 1)
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t u_ref = 4130464394;
    MrgDengLin2State obj = {.x = {1, 2}};
    uint32_t u;
    for (long i = 0; i < 1000000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("u = %lu, u_ref = %lu\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("MRG-DengLin-2", run_self_test)
