/**
 * @file rwc64.c
 * @brief
 * @details Its period is around 2^244; the RWC generator is a specialized form
 * of LCG (linear congruential generator) with prime modulus. It is based
 * on the next recurrent formula:
 *
 * \f[
 * u_i = a_2 x_{i-2} + a_3 x_{i-3} + c_{i-1}
 * \f]
 *
 * \f[
 * c_i = \left\lfloor \dfrac{u_i}{b} \right\rfloor;~
 * x_i = u_i \mod b
 * \f]
 *
 * It corresponds to the next multplicative congruential generator:
 *
 * \f[
 * \begin{array}{l}
 * m=a_3 b^3 + a_2 b^2 - 1\\
 * a_\mathrm{MCG} = b^{-1} \mod m\\
 * x_\mathrm{MCG} = b^3 c + b^2\left(x_{-1} - a_2 x_{-3}\right) x_{-2} + bx_{-2} + x_{-3} \\
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
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RWC64 pseudorandom number generator state.
 */
typedef struct {
    uint64_t x; ///< x_(n-1)
    uint64_t y; ///< x_(n-2)
    uint64_t z; ///< x_(n-3)
    uint64_t c; ///< carry
} Rwc64State;


static inline uint64_t get_bits_raw(Rwc64State *obj)
{
    static const uint64_t a = 12345671234567586U;
    uint64_t new_hi = 0, new_lo = obj->y;
    unsigned_add128(&new_hi, &new_lo, obj->z);
    umuladd_128x128p64w(0, a, &new_hi, &new_lo, obj->c);
    obj->z = obj->y;
    obj->y = obj->x;
    obj->x = new_lo;
    obj->c = new_hi;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc64State *obj = intf->malloc(sizeof(Rwc64State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->c = 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Rwc64State obj = {.x = 12345678, .y = 87654321, .z = 12345, .c = 1};
    const uint64_t u_ref = 0xA8292E74B94559A7;
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint64_t u = get_bits_raw(&obj);
    intf->printf("c = 0x%llX; x = 0x%llX; y = 0x%llX; z = 0x%llX\n",
        (unsigned long long) obj.c, (unsigned long long) obj.x,
        (unsigned long long) obj.y, (unsigned long long) obj.z);
    intf->printf("Out=0x%llX; ref=0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref ? 1 : 0;
    
}


MAKE_UINT64_PRNG("rwc64", run_self_test)
