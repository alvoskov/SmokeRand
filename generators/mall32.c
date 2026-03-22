/**
 * @file mall32.c
 * @brief "Mother-of-all" 32-bit PRNG by G. Marsaglia from DIEHARD CD-ROM.
 * @details It is a MWC/RWC (multiply-with-carry/recursion-with-carry)
 * generator: a MCG (multiplicative congruential generator) with a specific
 * for of prime modulus that allows an efficient implementation. Its period
 * is around 2^158. It is also theoretically possible to make a jump function.
 *
 * See Eq.6 in the article by Goresky and Klapper to map it to the LCG.
 *
 * References:
 *
 * 1. https://web.archive.org/web/20160125103112/http://stat.fsu.edu/pub/diehard/
 * 2. https://web.archive.org/web/20070402163410/http://www.cs.hku.hk/%7Ediehard/cdrom/
 * 3. https://pbcrypto.basicaware.de/ShowAlgorithm.aspx?id=45
 * 4. https://www.stat.berkeley.edu/~spector/s243/mother.c
 * 5. M. Goresky, A. Klapper. Efficient multiply-with-carry random number
 *    generators with maximal period // ACM Trans. Model. Comput. Simul. 2003.
 *    V. 13. N 4. P. 310-321. https://doi.org/10.1145/945511.945514
 *
 * Python 3.x script for parameters check:
 *
 *    import sympy, math
 *    b = 2**32
 *    m = 2111111111*b**4 + 1492*b**3 + 1776*b**2 + 5115*b - 1
 *    print(sympy.isprime(m), sympy.isprime((m-1)//2))
 *    period = sympy.n_order(b, m)
 *    print(period, math.log2(period))
 *
 * @copyright The "Mother-of-all" 32-bit PRNG was developed by G. Marsaglia.
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

/**
 * @brief Mother-of-all PRNG (32-bit version from DIEHARD CD-ROM) state.
 */
typedef struct {
    uint32_t x[4]; ///< (x(n-4), x(n-3), x(n-2), x(n-1)
    uint32_t c;    ///< carry
} Mall32State;


static inline uint64_t get_bits_raw(Mall32State *obj)
{
    const uint64_t prod = 2111111111U * (uint64_t)obj->x[0] + // x(n-4)
        1492U * (uint64_t)obj->x[1] + // x(n-3)
        1776U * (uint64_t)obj->x[2] + // x(n-2)
        5115U * (uint64_t)obj->x[3] + (uint64_t)obj->c; // x(n-1), c
    obj->x[0] = obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = (uint32_t) prod;
    obj->c = (uint32_t) (prod >> 32);
    return obj->x[3];
}


static void *create(const CallerAPI *intf)
{
    Mall32State *obj = intf->malloc(sizeof(Mall32State));    
    obj->c = 12345U;
    seeds_to_array_u32(intf, obj->x, 4);    
    return obj;
}

/**
 * @brief An internal self-test with values obtained from the Python 3.x
 * script (see the `..\misc\rwc.py` file)
 */
static int run_self_test(const CallerAPI *intf)
{
    Mall32State obj = {.x = {1234, 5678, 8765, 4321}, .c = 12345};
    const uint32_t u_ref = 0x59AFBCCF;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint32_t u = (uint32_t) get_bits_raw(&obj);
    intf->printf("Out=0x%lX; ref=0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref ? 1 : 0;
    
}


MAKE_UINT32_PRNG("Mall32", run_self_test)
