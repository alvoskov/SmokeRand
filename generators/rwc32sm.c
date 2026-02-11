/**
 * @file rwc32sm.c
 * @brief
 * @details Its period is around 2^93; the RWC generator is a specialized form
 * of LCG (linear congruential generator) with prime modulus. It is based
 * on the next recurrent formula:
 *
 * \f[
 * u_i = a_1 x_{i-1} + a_2 x_{i-2} + c_{i-1}
 * \f]
 *
 * \f[
 * c_i = \left\lfloor \dfrac{u_i}{b} \right\rfloor;~
 * x_i = u_i \mod b
 * \f]
 *
 * It corresponds to the next multplicative congruential generator:
 *
 * \f]
 * \begin{array}{l}
 * m=a_2b^2 + a_1b - 1\\
 * a_\mathrm{MCG} = b^{-1} \mod m\\
 * x_\mathrm{MCG} = b^2 c + x_{-2} + b\left(x_{-1} - a_1 x_{-2}\right) \\
 * \end{array}
 * \f]
 *
 * References:
 *
 * 1. https://www.stat.berkeley.edu/~spector/s243/mother.c
 * 2. M. Goresky, A. Klapper. Efficient multiply-with-carry random number
 *    generators with maximal period // ACM Trans. Model. Comput. Simul. 2003.
 *    V. 13. N 4. P. 310-321. https://doi.org/10.1145/945511.945514
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
    uint32_t y;
    uint32_t c;
} Rwc32SmState;



static inline uint64_t get_bits_raw(Rwc32SmState *obj)
{
    const uint64_t new = 1111111464ULL*((uint64_t)obj->x + obj->y) + obj->c;
    obj->y = obj->x;
    obj->x = (uint32_t) new;
    obj->c = (uint32_t) (new >> 32);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc32SmState *obj = intf->malloc(sizeof(Rwc32SmState));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->c = 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Rwc32SmState obj = {.x = 12345678, .y = 87654321, .c = 1};
    const uint32_t u_ref = 0xf6f730f6;
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint32_t u = (uint32_t) get_bits_raw(&obj);
    intf->printf("c = %lu; x = %lu; y = %lu\n",
        (unsigned long) obj.c, (unsigned long) obj.x, (unsigned long) obj.y);
    intf->printf("Out=0x%lX; ref=0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref ? 1 : 0;
    
}

MAKE_UINT32_PRNG("rwc32sm", run_self_test)
