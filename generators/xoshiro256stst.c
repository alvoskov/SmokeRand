/**
 * @file xoshiro256stst.c
 * @brief xoshiro256** pseudorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). This generator doesn't fail matrix rank and
 * linear complexity tests.
 * 
 * References:
 *
 * 1. https://prng.di.unimi.it/xoshiro256starstar.c
 *
 * @copyright
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief xoshiro256** PRNG state.
 */
typedef struct {
    uint64_t s[4];
} Xoshiro256StStState;


static inline uint64_t get_bits_raw(Xoshiro256StStState *state)
{
	const uint64_t result = rotl64(state->s[1] * 5, 7) * 9;
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
    Xoshiro256StStState *obj = intf->malloc(sizeof(Xoshiro256StStState));
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
        0x2AB042633A137B01, 0xF8323DB3041B8613,
        0x802773CD2BF2E6E7, 0xD647EAF01CBCD4BC,
        0xA1EE613636B2F629, 0x4A26D8D8F260DA9B,
        0x315D6923346B06F1, 0x5E8FF1BFE4345EFE
    };
    Xoshiro256StStState *obj = intf->malloc(sizeof(Xoshiro256StStState));
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


MAKE_UINT64_PRNG("xoshiro256**", run_self_test)
