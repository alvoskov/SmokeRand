/**
 * @file alfib_shared.c
 * @brief A shared library that implements the additive
 * Lagged Fibbonaci generator \f$ LFib(2^{64}, 607, 273, +) \f$.
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - 607} + X_{n - 273}
 * \f]
 * and returns higher 32 bits. The initial values in the ring buffer
 * are filled by the 64-bit PCG generator.
 *
 * Fails bspace32_1d and gap_inv512 tests.
 * 
 * Sources of parameters:
 *
 * 1. https://www.boost.org/doc/libs/master/boost/random/lagged_fibonacci.hpp
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

// Fails both birthday spacings and gap512 tests
#define LFIB_A 607
#define LFIB_B 273

typedef struct {
    uint64_t U[LFIB_A + 1]; ///< Ring buffer (U[0] is not used)
    int i;
    int j;
} ALFib_State;

static inline uint64_t get_bits_raw(void *state)
{
    ALFib_State *obj = state;
    uint64_t x = obj->U[obj->i] + obj->U[obj->j];
    obj->U[obj->i] = x;
    if(--obj->i == 0) obj->i = LFIB_A;
	if(--obj->j == 0) obj->j = LFIB_A;
    return x;
}

static void *create(const CallerAPI *intf)
{
    ALFib_State *obj = intf->malloc(sizeof(ALFib_State));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (size_t k = 1; k <= LFIB_A; k++) {    
        obj->U[k] = pcg_bits64(&state);
    }
    obj->i = LFIB_A; obj->j = LFIB_B;
    return (void *) obj;
}

MAKE_UINT64_PRNG("ALFib", NULL)
