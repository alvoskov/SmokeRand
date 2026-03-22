/**
 * @file pqrnd64.c
 * @brief A nonlinear 32-bit generator with an invertible mapping.
 * @details This PRNG is developed by Karl-Uwe Frank and is based
 * on a nonlinear invertible mapping. See details in the `pqrng32.c`
 * file.
 *
 * 1. http://www.freecx.co.uk/pqRNG/
 * 2. V. S. Anachin. Uniformly distributed sequences of p-adic integers //
 *    Mathematical Notes. 1994. V. 55. P. 109–133.
 *    https://doi.org/10.1007/BF02113290 
 *
 * @copyright pqrng algorithm was developed by Karl-Uwe Frank and is based
 * on invertible mappings suggested by V. S. Anashin.
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

typedef Lcg64State Pqrng64State;

static inline uint64_t get_bits_raw(Pqrng64State *obj)
{
    static const uint64_t r = 0x517CC1B727220A95; // (mod8 = 5)
    static const uint64_t p = 0x9E3779B97F4A7C13; // (mod8 = 3)
    obj->x = (obj->x ^ r) * p;
    return obj->x >> 32;
}


static void *create(const CallerAPI *intf)
{
    Pqrng64State *obj = intf->malloc(sizeof(Pqrng64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT32_PRNG("PQRNG64", NULL)
