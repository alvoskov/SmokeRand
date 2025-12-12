/**
 * @file a5rand.c
 * @brief a5rand generator is a nonlinear chaotic pseudorandom number
 * generator suggested by Aleksey Vaneev. The algorithm description and
 * official test vectors can be found at https://github.com/avaneev/komihash
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * @copyright The a5rand algorithm was developed by Aleksey Vannev.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief a5rand PRNG state.
 */
typedef struct {
    uint64_t st1;
    uint64_t st2;
} A5RandState;


static inline uint64_t get_bits_raw(A5RandState *state)
{
    static const uint64_t inc1 = 0x5555555555555555;
    static const uint64_t inc2 = 0xaaaaaaaaaaaaaaaa;
    state->st1 = unsigned_mul128(state->st1 + inc1, state->st2 + inc2, &state->st2);
    return state->st1 ^ state->st2;
}


static void *create(const CallerAPI *intf)
{
    A5RandState *obj = intf->malloc(sizeof(A5RandState));
    obj->st1 = intf->get_seed64();
    obj->st2 = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

/**
 * @brief An internal self-test based on 
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = { // Official test vectors
        0x2492492492492491, 0x83958cf072b19e08,
        0x1ae643aae6b8922e, 0xf463902672f2a1a0,
        0xf7a47a8942e378b5, 0x778d796d5f66470f,
        0x966ed0e1a9317374, 0xaea26585979bf755
    };
    A5RandState obj = {0, 0};
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint64_t u = get_bits_raw(&obj);
        intf->printf("Out: %16.16" PRIX64 "; ref: %16.16" PRIX64 "\n",
            u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("a5rand", run_self_test)
