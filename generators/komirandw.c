/**
 * @file komirandw.c
 * @brief Komirand-Weyl is a modification of Komirand generator that has an
 * additional linear component - discrete Weyl sequence, this modification
 * provides a period at least 2^64 and make the PRNG suitable for practical
 * applications. This modification was made by A.L. Voskov.
 * 
 * The original Komirand generator was suggested by Aleksey Vaneev. The
 * algorithm description and official test vectors can be found at
 * https://github.com/avaneev/komihash
 *
 * @copyright The komirand algorithm was developed by Aleksey Vannev.
 *
 * Addition of "discrete Weyl sequence" and reentrant plugin for SmokeRand:
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
 * @brief Komirand-Weyl PRNG state.
 */
typedef struct {
    uint64_t st1; ///< Nonlinear part
    uint64_t st2; ///< Nonlinear part
    uint64_t w;   ///< Linear part (discrete Weyl sequence)
} KomirandWeylState;

static inline uint64_t get_bits_raw(KomirandWeylState *state)
{
    static const uint64_t inc = 0x9E3779B97F4A7C15;
    uint64_t s1 = state->st1, s2 = state->st2, mul_hi;
    const uint64_t mul_lo = unsigned_mul128(s1, s2, &mul_hi);
    s2 += mul_hi + (state->w += inc);
    s1 = mul_lo ^ s2;
    state->st1 = s1;
    state->st2 = s2;
    return s1;
}

static void *create(const CallerAPI *intf)
{
    KomirandWeylState *obj = intf->malloc(sizeof(KomirandWeylState));
    obj->st1 = intf->get_seed64();
    obj->st2 = intf->get_seed64();
    obj->w   = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = { // Custom test vectors
        0x9E3779B97F4A7C15, 0xE32ADE5030DCA19E,
        0xAFFC46FCEBE757A9, 0x9273619EC0DDEF35,
        0xC4C7AD4E190018B8, 0xC85BC9BB2005C769,
        0x07C8F583A6415C05, 0x97243F6918FA00CE
    };
    KomirandWeylState obj = {0, 0, 0};
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

MAKE_UINT64_PRNG("KomirandWeyl", run_self_test)
