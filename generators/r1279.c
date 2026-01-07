/**
 * @file r1279.c
 * @brief Implementation of XOR-based lagged Fibbonaci generator
 * \f$ LFib(2^{32}, 1279, 1063 ) \f$.
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - 1279} XOR X_{n - 1063}
 * \f]
 *
 * This generator fails gap test, linear complexity and matrix rank tests.
 * Similar generators caused problems in 2D Ising model computation by
 * Monte-Carlo method. So it mustn't be used as a general purpose generator.
 * 
 * References:
 *
 * 1. https://doi.org/10.1016/j.cpc.2007.10.002
 * 2. https://doi.org/10.1103/PhysRevLett.69.3382
 * 3. https://doi.org/10.1103/PhysRevE.52.3205
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define RGEN_A 1279
#define RGEN_B 1063


typedef struct {
    uint32_t x[RGEN_A + 1]; ///< Ring buffer (u[0] is not used)
    int i;
    int j;
} RGenState;

static inline uint64_t get_bits_raw(RGenState *obj)
{
    const uint32_t x = obj->x[obj->i] ^ obj->x[obj->j];
    obj->x[obj->i] = x;
    if (--obj->i == 0) obj->i = RGEN_A;
	if (--obj->j == 0) obj->j = RGEN_A;
    return x;
}

static void *create(const CallerAPI *intf)
{
    RGenState *obj = intf->malloc(sizeof(RGenState));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (size_t k = 1; k <= RGEN_A; k++) {    
        obj->x[k] = (uint32_t) pcg_bits64(&state);
    }
    obj->i = RGEN_A; obj->j = RGEN_B;
    return obj;
}

MAKE_UINT32_PRNG("R1279", NULL)
