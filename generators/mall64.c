/**
 * @file mall64.c
 * @brief "Mother-of-all" 64-bit PRNG made by A.L. Voskov is a modification
 * of the similar 32-bit PRNG by G. Marsaglia from DIEHARD CD-ROM.
 * @details It is a MWC/RWC (multiply-with-carry/recursion-with-carry)
 * generator: a MCG (multiplicative congruential generator) with a specific
 * for of prime modulus that allows an efficient implementation. Its period
 * is around 2^319. It is also theoretically possible to make a jump function.
 *
 * See Eq.6 in the article by Goresky and Klapper to map it to the LCG.
 *
 * It is a fairly slow generator (around 1-1.5 cpb) and is interesting more
 * as an experiment.
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
 *    b = 2**64
 *    m = 18000_69069_69069_69069*b**4 + 69069_69069*b**3 + \
 *        1147_1147*b**2 + 90050_90050*b - 1
 *    print(sympy.isprime(m), sympy.isprime((m-1)//2))
 *    period = sympy.n_order(b, m)
 *    print(period, math.log2(period))
 *
 * @copyright The "Mother-of-all" 32-bit PRNG was developed by G. Marsaglia.
 *
 * 64-bit modification and its reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 64-bit adaptation of Mother-of-all PRNG (based on its 32-bit
 * version from DIEHARD CD-ROM) state.
 */
typedef struct {
    uint64_t x[4]; ///< (x(n-4), x(n-3), x(n-2), x(n-1)
    uint64_t c;    ///< carry
} Mall64State;


static inline uint64_t get_bits_raw(Mall64State *obj)
{
    uint64_t sum_hi, sum_lo, prod_hi, prod_lo;
    sum_lo  = unsigned_mul128(18000690696906969069U, obj->x[0], &sum_hi);  // x(n-4)
    unsigned_add128(&sum_hi, &sum_lo, obj->c);                             // carry
    prod_lo = unsigned_mul128(6906969069U,           obj->x[1], &prod_hi); // x(n-3)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    prod_lo = unsigned_mul128(11471147U,             obj->x[2], &prod_hi); // x(n-2)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    prod_lo = unsigned_mul128(9005090050U,           obj->x[3], &prod_hi); // x(n-1)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;    
    obj->x[0] = obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = sum_lo;
    obj->c = sum_hi;
    return sum_lo;
}


static void *create(const CallerAPI *intf)
{
    Mall64State *obj = intf->malloc(sizeof(Mall64State));
    obj->c = 1234567890U;
    seeds_to_array_u64(intf, obj->x, 4);
    return obj;
}

/**
 * @brief An internal self-test with values obtained from the Python 3.x
 * script (see the `..\misc\rwc.py` file)
 */
static int run_self_test(const CallerAPI *intf)
{
    Mall64State obj = {.x = {1234, 5678, 8765, 4321}, .c = 12345};
    const uint64_t u_ref = 0x2D0DEFE653124B9A;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint64_t u = get_bits_raw(&obj);
    intf->printf("Out=0x%llX; ref=0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref ? 1 : 0;
    
}


MAKE_UINT64_PRNG("Mall64", run_self_test)
