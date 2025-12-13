/**
 * @file xoshiro256p.c
 * @brief xoshiro256+ pseudorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). Its lowest bit has low linear complexity.
 * It fails `linearcomp_low`, `matrixrank_4096_low`, `matrixrank_8192_low` and
 * `matrixrank_8192` tests.
 *
 * Reference:
 * 
 * 1. https://prng.di.unimi.it/xoshiro256plus.c
 *
 * @copyright
 * An initial wrapper for SmokeRand: (C) 2025 Scott Baker
 *
 * Refactoring with addition of internal self-tests:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief xoshiro256+ PRNG state.
 */
typedef struct {
    uint64_t s[4];
} Xoshiro256PState;


static inline uint64_t get_bits_raw(Xoshiro256PState *state)
{
	const uint64_t result = state->s[0] + state->s[3];
	const uint64_t t = state->s[1] << 17;
	state->s[2] ^= state->s[0];
	state->s[3] ^= state->s[1];
	state->s[1] ^= state->s[2];
	state->s[0] ^= state->s[3];
	state->s[2] ^= t;
	state->s[3] = rotl64(state->s[3], 45);
	return result;
}


static void *create(const CallerAPI *intf)
{
    Xoshiro256PState *obj = intf->malloc(sizeof(Xoshiro256PState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    obj->s[2] = intf->get_seed64();
    obj->s[3] = intf->get_seed64();
    if (obj->s[0] == 0 && obj->s[1] == 0 && obj->s[2] == 0 && obj->s[3] == 0) {
        obj->s[0] = 0x12345678;
    }
    return obj;
}

/**
 * @brief Test vectors were obtained from the reference implementation.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[8] = {
        0x0EC5A3669AE0BB1E, 0x38E89B4AA8E33BCF,
        0x258863CE8745EA1C, 0x5D109B4F3EF83B31,
        0xF71EBB159A016557, 0x948A8EDE055E2AD9,
        0xFAAE897DBF1DB67C, 0x33EDEAF7270C672F
    };
    Xoshiro256PState *obj = intf->malloc(sizeof(Xoshiro256PState));
    obj->s[0] = 0x12345678; obj->s[1] = 1;
    obj->s[2] = 2;          obj->s[3] = 3;
    for (int i = 0; i < 1024; i++) {
        (void) get_bits_raw(obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint64_t u = get_bits_raw(obj);
        intf->printf("Out = %16.16" PRIX64 "; ref = %16.16" PRIX64 "\n",
            u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("xoshiro256+", run_self_test)
