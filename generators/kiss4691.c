/**
 * @file kiss4691.c
 * @brief KISS4691 generator by G. Marsaglia.
 * @details A part of combined KISS4691 generator that includes SuperDuper
 * (LCG32 + xorshift32) and MWC4691 subgenerators. It allows to hide numerous
 * flaws of SuperDuper and failure of the `gap16_count0` test of MWC4691.
 *
 * Description from G.Marsaglia:
 *
 *     The MWC4691 sequence x[n]=8193*x[n-4691]+carry mod b=2^32
 *     is based on p=8193*b^4691-1, period ~ 2^150124 or 10^45192
 *     For the MWC (Multiply-With-Carry) process, given a current
 *     x and c, the new x is (8193*x+c) mod b,
 *     the new c is the integer part of (8193*x+c)/b.
 *
 * Marsaglia initalizes MWC4691 with some modification of SuperDuper PRNG.
 * The 8193 multiplier is 0x2001 in hexadecimal representation, that allows
 * to replace the multiplication and modulo operators into some bit hacks.
 *
 * References:
 * 
 * 1. G. Marsaglia. KISS4691, a potentially top-ranked RNG.
 * https://www.thecodingforums.com/threads/kiss4691-a-potentially-top-ranked-rng.729111/
 * @copyright The KISS4961 algorithm was developed by G. Marsaglia.
 *
 * Reentrant C99 version for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS4691 state.
 */
typedef struct {
    uint32_t q[4691];
    unsigned int c;
    int j;
    uint32_t xcng;
    uint32_t xs;
} Kiss4691State;


static inline uint32_t Kiss4691State_mwc_iter(Kiss4691State *obj)
{
    obj->j = (obj->j < 4690) ? obj->j + 1 : 0;
    uint32_t x = obj->q[obj->j];
    uint32_t t = (x << 13) + obj->c + x;
    obj->c = (t < x) + (x >> 19);
    obj->q[obj->j] = t;
    return obj->q[obj->j];
}

static inline uint32_t Kiss4691State_supdup_iter(Kiss4691State *obj)
{
    obj->xcng = 69069u * obj->xcng + 123u;
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 5;
    return obj->xcng + obj->xs;
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t sd  = Kiss4691State_supdup_iter(state);
    uint32_t mwc = Kiss4691State_mwc_iter(state);
    return mwc + sd;
}

static void Kiss4691State_init(Kiss4691State *obj, uint32_t xcng, uint32_t xs)
{
    obj->xcng = xcng;
    obj->xs = (xs != 0) ? xs : 0x12345678;
    for (int i = 0; i < 4691; i++) {
        obj->q[i] = Kiss4691State_supdup_iter(obj);
    }
    obj->c = 0;
    obj->j = 4691;
}


static void *create(const CallerAPI *intf)
{
    Kiss4691State *obj = intf->malloc(sizeof(Kiss4691State));
    uint32_t xcng, xs;
    seed64_to_2x32(intf, &xcng, &xs);
    Kiss4691State_init(obj, xcng, xs);
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    const uint32_t x_mwc_ref = 3740121002, x_kiss_ref = 2224631993;
    uint32_t x_mwc, x_kiss;
    Kiss4691State *obj = intf->malloc(sizeof(Kiss4691State));
    Kiss4691State_init(obj, 362436069, 521288629); 
    for (unsigned long i = 0; i < 1000000000; i++) {
        x_mwc = (uint32_t) Kiss4691State_mwc_iter(obj);
    }
    intf->printf("x_mwc  = %22u; x_mwc_ref  = %22u\n", x_mwc, x_mwc_ref);
    for (unsigned long i = 0; i < 1000000000; i++) {
        x_kiss = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("x_kiss = %22u; x_kiss_ref = %22u\n", x_kiss, x_kiss_ref);
    intf->free(obj);
    return x_mwc == x_mwc_ref && x_kiss == x_kiss_ref;
}


MAKE_UINT32_PRNG("Kiss4691", run_self_test)

