/**
 * @file mwc4691.c
 * @brief MWC4691 generator by G. Marsaglia.
 * @details A part of combined KISS4691 generator. Passes BigCrush but
 * not PractRand or gjrand. It also fails `gap16_count0` test from SmokeRand
 * `brief`, `default` and `full` tests batteries (taken from gjrand).
 *
 * Description from G.Marsaglia:
 *
 *     The MWC4691 sequence x[n]=8193*x[n-4691]+carry mod b=2^32
 *     is based on p=8193*b^4691-1, period ~ 2^150124 or 10^45192
 *     For the MWC (Multiply-With-Carry) process, given a current
 *     x and c, the new x is (8193*x+c) mod b,
 *     the new c is the integer part of (8193*x+c)/b.
 *
 * Marsaglia initalizes MWC4691 with some modification of SuperDuper PRNG, its
 * replacement to TRNG doesn't influence on `gap16_count0` test failure.
 * The 8193 multiplier is 0x2001 in hexadecimal representation, that allows
 * to replace the multiplication and modulo operators into some bit hacks.
 *
 * This generator is rather useful for testing the `gap16_count0` because it
 * is the only test from SmokeRand batteries that it fails.
 *
 * References:
 * 
 * 1. G. Marsaglia. KISS4691, a potentially top-ranked RNG.
 * https://www.thecodingforums.com/threads/kiss4691-a-potentially-top-ranked-rng.729111/
 * @copyright The MWC4961 algorithm was developed by G. Marsaglia.
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC4691 state.
 */
typedef struct {
    uint32_t Q[4691];
    int c;
    int j;
} Mwc4691State;

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t t, x;
    Mwc4691State *obj = state;
    obj->j = (obj->j < 4690) ? obj->j + 1 : 0;
    x = obj->Q[obj->j];
    t = (x << 13) + obj->c + x;
    obj->c = (t < x) + (x >> 19);
    obj->Q[obj->j] = t;
    return obj->Q[obj->j];
}

static void Mwc4691State_init(Mwc4691State *obj, uint32_t xcng, uint32_t xs)
{
    for (int i = 0; i < 4691; i++) {
        xcng = 69069 * xcng + 123;
        xs ^= (xs << 13);
        xs ^= (xs >> 17);
        xs ^= (xs << 5);
        obj->Q[i] = xcng + xs;
    }
    obj->c = 0;
    obj->j = 4691;
}


static void *create(const CallerAPI *intf)
{
    Mwc4691State *obj = intf->malloc(sizeof(Mwc4691State));
    uint64_t seed = intf->get_seed64();
    Mwc4691State_init(obj, seed >> 32, (seed & 0xFFFFFFFF) | 1);
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    uint32_t x, x_ref = 3740121002;
    Mwc4691State *obj = intf->malloc(sizeof(Mwc4691State));
    Mwc4691State_init(obj, 362436069, 521288629); 
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("x = %22u; x_ref = %22u\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}


MAKE_UINT32_PRNG("Mwc4691", run_self_test)

