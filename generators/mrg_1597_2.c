/**
 * @file mrg_1597_2.c
 * @brief MRG-1597-2 multiple recursive generator with 31-bit output, very high
 * period and good quality.
 * @details It uses the next recurrent formula:
 *
 * \f[
 * X i = 1057217510 X_{i-1} + 1066409146 X_{i-1597} \mod \left(2^{31} - 1\right)
 * \f]
 *
 * References:
 *
 * 1. Deng, Lih-Yuan. Efficient and portable multiple recursive generators of
 *    large order // ACM Trans. Model. Comput. Simul. 2005. V 15. N 1. P.1-13.
 *    https://doi.org/10.1145/1044322.1044323
 *
 * @copyright The MRG-1597-2 algorithm is developed Deng L.-Y.
 * 
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define MRG_1597_2_N  1597

typedef struct {
    uint32_t x[MRG_1597_2_N];
    int i;
    int j;
} Mrg1597x2State;


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


static inline uint64_t get_bits_raw(Mrg1597x2State *obj)
{
    static const uint64_t a[2] = {1057217510, 1066409146};
    const uint32_t x = mod_mp31(a[0]*obj->x[obj->j] + a[1]*obj->x[obj->i]);
    obj->x[obj->i] = x;
    if (++obj->j == MRG_1597_2_N) obj->j = 0;
    if (++obj->i == MRG_1597_2_N) obj->i = 0;
    return x << 1;
}


static void *create(const CallerAPI *intf)
{
    Mrg1597x2State *obj = intf->malloc(sizeof(Mrg1597x2State));
    expand_seed64_to_u32(obj->x, MRG_1597_2_N, intf->get_seed64());
    for (int i = 0; i < MRG_1597_2_N; i++) {
        obj->x[i] &= 0x7FFFFFFF;
        if (obj->x[i] == 0 || obj->x[i] == 0x7FFFFFFF) {
            obj->x[i] = 1;
        }
    }
    obj->i = 0;
    obj->j = MRG_1597_2_N - 1;
    return obj;
}

/**
 * @brief The test is based on the values obtained from Python script.
 * @details The used script:
 *
 *    x = list(range(0, 1597))
 *    for i in range(0, 100_000):
 *        x_new = (1057217510*x[1596] + 1066409146*x[0]) % (2**31 - 1)
 *        x = x[1:] + [x_new]
 *    print(x[-1] << 1)
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t u_ref = 3857273820;
    Mrg1597x2State *obj = create(intf);
    for (unsigned int i = 0; i < MRG_1597_2_N; i++) {
        obj->x[i] = i;
    }
    uint32_t u;
    for (long i = 0; i < 100000; i++) {
        u = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("u = %lu, u_ref = %lu\n",
        (unsigned long) u, (unsigned long) u_ref);
    intf->free(obj);
    return u == u_ref;
}

MAKE_UINT32_PRNG("mrg_1597_2", run_self_test)

#undef MRG_1597_2_N
