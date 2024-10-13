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
 * Sources of parameters:
 *
 * 1. https://www.boost.org/doc/libs/master/boost/random/lagged_fibonacci.hpp
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

// Fails both birthday spacings and gap512 tests
#define LFIB_A 607
#define LFIB_B 273


// Still fails birthday spacings test, gap512 is "suspiciuos"
//#define LFIB_A 1279
//#define LFIB_B 418

// Still fails birthday spacings test, gap512 is passed
//#define LFIB_A 2281
//#define LFIB_B 1252


typedef struct {
    uint64_t U[LFIB_A + 1]; ///< Ring buffer (only values 1..17 are used)
    int i;
    int j;
} ALFib_State;

static uint64_t get_bits(void *state)
{
    ALFib_State *obj = state;
    uint64_t x = obj->U[obj->i] + obj->U[obj->j];
    obj->U[obj->i] = x;
    if(--obj->i == 0) obj->i = LFIB_A;
	if(--obj->j == 0) obj->j = LFIB_A;
    return x >> 32;
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

MAKE_UINT32_PRNG("ALFib", NULL)
