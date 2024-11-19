/**
 * @file alfib_par_shared.c
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


// Still fails birthday spacings test, gap512 is "suspiciuos"
//#define LFIB_A 1279
//#define LFIB_B 418

// Still fails birthday spacings test, gap512 is passed
//#define LFIB_A 2281
//#define LFIB_B 1252

//#define LFIB_A 4423
//#define LFIB_B 2098


typedef struct {
    int is_additive;
    int a;
    int b;
    int i;
    int j;
    uint64_t *u; ///< Ring buffer (U[0] is not used)
} ALFibDyn_State;

static inline uint64_t get_bits_raw(void *state)
{
    ALFibDyn_State *obj = state;
    uint64_t x;
    if (obj->is_additive) {
        x = obj->u[obj->i] + obj->u[obj->j];
    } else {
        x = obj->u[obj->i] - obj->u[obj->j];
    }
    obj->u[obj->i] = x;
    if(--obj->i == 0) obj->i = obj->a;
	if(--obj->j == 0) obj->j = obj->a;
    return x >> 32;
}


static ALFibDyn_State parse_parameters(const CallerAPI *intf)
{
    ALFibDyn_State obj;
    const char *param = intf->get_param();
    if (!intf->strcmp("55+", param)) {
        obj.a = 55; obj.b = 24; obj.is_additive = 1;
    } else if (!intf->strcmp("55-", param)) {
        obj.a = 55; obj.b = 24; obj.is_additive = 0;
    } else if (!intf->strcmp("607+", param) || !intf->strcmp("", param)) {
        obj.a = 607; obj.b = 273; obj.is_additive = 1;
    } else if (!intf->strcmp("607-", param)) {
        obj.a = 607; obj.b = 273; obj.is_additive = 0;
    } else if (!intf->strcmp("1279+", param)) {
        obj.a = 1279; obj.b = 418; obj.is_additive = 1;
    } else if (!intf->strcmp("1279-", param)) {
        obj.a = 1279; obj.b = 418; obj.is_additive = 0;
    } else if (!intf->strcmp("2281+", param)) {
        obj.a = 2281; obj.b = 1252; obj.is_additive = 1;
    } else if (!intf->strcmp("2281-", param)) {
        obj.a = 2281; obj.b = 1252; obj.is_additive = 0;
    } else if (!intf->strcmp("3217+", param)) {
        obj.a = 3217; obj.b = 576; obj.is_additive = 1;
    } else if (!intf->strcmp("3217-", param)) {
        obj.a = 3217; obj.b = 576; obj.is_additive = 0;
    } else if (!intf->strcmp("4423+", param)) {
        obj.a = 4423; obj.b = 2098; obj.is_additive = 1;
    } else if (!intf->strcmp("4423-", param)) {
        obj.a = 4423; obj.b = 2098; obj.is_additive = 0;
    } else {
        obj.a = 0; obj.b = 0; obj.is_additive = 0;
    }
    return obj;
}

static void *create(const CallerAPI *intf)
{
    ALFibDyn_State par = parse_parameters(intf);
    if (par.a == 0) {
        intf->printf("Unknown parameter %s\n", intf->get_param());
        return NULL;
    }
    intf->printf("LFib(%d,%d,%s)\n", par.a, par.b,
        par.is_additive ? "+" : "-");
    // Allocate buffers
    size_t len = sizeof(ALFibDyn_State) + (par.a + 2) * sizeof(uint64_t);    
    ALFibDyn_State *obj = intf->malloc(len);
    *obj = par;
    obj->u = (uint64_t *) ( (char *) obj + sizeof(ALFibDyn_State) );
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int k = 1; k <= obj->a; k++) {
        obj->u[k] = pcg_bits64(&state);
    }
    obj->i = obj->a; obj->j = obj->b;
    return (void *) obj;
}

MAKE_UINT32_PRNG("ALFib", NULL)
