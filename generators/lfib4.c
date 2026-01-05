/**
 * @file lfib4.c 
 * @brief An implementation of LFIB4 generator by G.Marsaglia.
 * Fails some statistical tests due to low quality of lower bits. 
 * @details References:
 *
 * 1. http://www.cse.yorku.ca/~oz/marsaglia-rng.html
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t t[256];
    uint8_t c;
} LFib4State;

static inline uint64_t get_bits_raw(LFib4State *obj)
{
    uint32_t *t = obj->t;
    obj->c++;
    const uint8_t c1 = (uint8_t) (obj->c + 58U);
    const uint8_t c2 = (uint8_t) (obj->c + 119U);
    const uint8_t c3 = (uint8_t) (obj->c + 178U);
    t[obj->c] += t[c1] + t[c2] + t[c3];
    return t[obj->c];
}

/**
 * @param jcong0 Marsaglia's default is 12345
 */
static void LFib4State_init(LFib4State *obj, uint32_t jcong0)
{
    uint32_t z = 12345, w = 65435, xs = 34221, jcong = jcong0;
    // Use Marsaglia's original procedure for LFIB4 initailization
    //uint64_t state = intf->get_seed64();
    for (int i = 0; i < 256; i++) {
        // KISS99 iteration
        z = 36969U * (z & 65535) + (z >> 16);
        w = 18000U * (w & 65535) + (w >> 16);
        jcong = 69069U * jcong + 1234567U;
        xs ^= (xs << 17); // Note: bad SHR3 from original is kept here!
        xs ^= (xs >> 13);
        xs ^= (xs << 5);
        const uint32_t mwc = (z << 16) + w;
        const uint32_t kiss = (mwc ^ jcong) + xs;
        // Write to LFIB4 table
        obj->t[i] = kiss;
    }
    obj->c = 0;
}

static void *create(const CallerAPI *intf)
{
    LFib4State *obj = intf->malloc(sizeof(LFib4State));
    LFib4State_init(obj, 12345);
    return (void *) obj;
}

static int run_self_test(const CallerAPI *intf)
{
    uint32_t x, x_ref = 1064612766;
    LFib4State *obj = intf->malloc(sizeof(LFib4State));
    LFib4State_init(obj, 12345); 
    for (unsigned long i = 0; i < 1000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("x = %22u; x_ref = %22u\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}



MAKE_UINT32_PRNG("LFib4", run_self_test)
