/**
 * @file xoshiro256pp.c
 * @brief xoshiro256++ PRNG: https://prng.di.unimi.it/xoshiro256plusplus.c
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief PRNG state.
 */
typedef struct {
    uint64_t s[4];
} Xoshiro256PPState;

static inline uint64_t get_bits_raw(Xoshiro256PPState *state)
{
	const uint64_t result = rotl64(state->s[0] + state->s[3], 23) + state->s[0];
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
    Xoshiro256PPState *obj = intf->malloc(sizeof(Xoshiro256PPState));
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
        0x2646C3A1477F37A3, 0x3A06301F72C769B1,
        0x36038B81CA970758, 0xB222AEE53C5D5F99,
        0x07CE6CD7FA209703, 0x4C80C9E3834B050C,
        0x1D9667DFE521B7BC, 0x2DFEF38F081A6360
    };
    Xoshiro256PPState *obj = intf->malloc(sizeof(Xoshiro256PPState));
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

MAKE_UINT64_PRNG("xoshiro256++", run_self_test)
