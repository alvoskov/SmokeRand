/**
 * @file r250.c
 * @brief Implementation of XOR-based lagged Fibbonaci generator
 * \f$ LFib(2^{32}, 250, 103 ) \f$.
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - 250} XOR X_{n - 103}
 * \f]
 *
 * This generator fails gap test, linear complexity and matrix rank tests.
 * Similar generators caused problems in 2D Ising model computation by
 * Monte-Carlo method. So it mustn't be used as a general purpose generator.
 *
 *        # Test name                    xemp              p Interpretation  
 *    -----------------------------------------------------------------------
 *        5 bspace32_1d                 37016      2.99e-117 FAIL            
 *        6 bspace32_1d_high            37177      6.07e-126 FAIL            
 *       26 gap_inv512                26512.8              0 FAIL            
 *       27 gap_inv1024                199173              0 FAIL            
 *       28 gap16_count0                   40              0 FAIL            
 *       29 hamming_distr             30.4796      2.43e-204 FAIL            
 *       37 linearcomp_high               250              0 FAIL            
 *       38 linearcomp_mid                250              0 FAIL            
 *       39 linearcomp_low                250              0 FAIL            
 *       40 matrixrank_4096            434.11       5.42e-95 FAIL            
 *       41 matrixrank_4096_low8       434.11       5.42e-95 FAIL            
 *       42 matrixrank_8192            434.11       5.42e-95 FAIL            
 *       43 matrixrank_8192_low8       434.11       5.42e-95 FAIL            
 *    -----------------------------------------------------------------------
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

#define RGEN_A 250
#define RGEN_B 103


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

MAKE_UINT32_PRNG("R250", NULL)
