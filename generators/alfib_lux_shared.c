/**
 * @file alfib_lux_shared.c
 * @brief A shared library that implements the additive
 * Lagged Fibbonaci generator \f$ LFib(2^{32}, 100, 37, +)[100,1009] \f$.
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - 607} + X_{n - 273}
 * \f]
 * and returns ALL 32 bits. The initial values in the ring buffer are filled
 * by the 64-bit PCG generator.
 *
 * Fails birthday spacings and gap tests, the lowest bit has low linear
 * complexity.
 * 
 * Sources of parameters:
 *
 * 1. TAOCP2
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define LFIB_TOTAL 1009
#define LFIB_A 100
#define LFIB_B 37

typedef struct {
    uint32_t x[LFIB_A];
    int i;
    int j;
    int pos;
} ALFib_State;

static inline uint64_t ALFib_State_get_bits(ALFib_State *obj)
{
    uint32_t x = obj->x[obj->i] + obj->x[obj->j];
    obj->x[obj->i] = x;
    if (++obj->i == LFIB_A) obj->i = 0;
	if (++obj->j == LFIB_A) obj->j = 0;
    return x;
}

static inline uint64_t get_bits_raw(void *state)
{
    ALFib_State *obj = state;
    uint32_t x = ALFib_State_get_bits(obj);
    if (++obj->pos == LFIB_A) {
        for (int i = 0; i < LFIB_TOTAL - LFIB_A; i++) {
            (void) ALFib_State_get_bits(obj);
        }
        obj->pos = 0;
    }
    return x;
}

static void *create(const CallerAPI *intf)
{
    ALFib_State *obj = intf->malloc(sizeof(ALFib_State));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int k = 0; k < LFIB_A; k++) {    
        obj->x[k] = pcg_bits64(&state);
    }
    obj->i = 0; obj->j = LFIB_A - LFIB_B;
    obj->pos = 0;
    return (void *) obj;
}

MAKE_UINT32_PRNG("ALFibLux", NULL)
