/**
 * @file a5randw.c
 * @brief a5rand-Weyl is a modification of a5rand generator that has an
 * additional linear component - discrete Weyl sequence, this modification
 * provides a period at least 2^64 and makes the PRNG suitable for practical
 * applications. This modification was made by A.L. Voskov.
 * 
 * The original a5rand generator was suggested by Aleksey Vaneev. The
 * algorithm description and official test vectors can be found at
 * https://github.com/avaneev/komihash
 *
 * @copyright The a5rand algorithm was developed by Aleksey Vannev.
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
 * @brief a5rand-weyl PRNG state.
 */
typedef struct {
    uint64_t st1;
    uint64_t st2;
    uint64_t w;
} A5RandWeylState;


static inline uint64_t get_bits_raw(A5RandWeylState *obj)
{
    static const uint64_t inc1 = 0x5555555555555555;
    obj->w += 0x9E3779B97F4A7C15;
    obj->st1 = unsigned_mul128(obj->st1 + inc1, obj->st2 + obj->w, &obj->st2);
    return obj->st1 ^ obj->st2;
}


static void *create(const CallerAPI *intf)
{
    A5RandWeylState *obj = intf->malloc(sizeof(A5RandWeylState));
    obj->st1 = intf->get_seed64();
    obj->st2 = intf->get_seed64();
    obj->w   = intf->get_seed64();
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

/**
 * @brief An internal self-test
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = {
        0x14250451008902A5, 0xDCFCC934621465C8,
        0xED5DF26967142930, 0xC5F1200DFF592138,
        0x75B488F5FC293E02, 0x901FED8B6B18ADD3,
        0x18D7786413A4D922, 0x840B6E159ADB90D5
    };
    A5RandWeylState obj = {0, 0, 0};
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

MAKE_UINT64_PRNG("a5rand-Weyl", run_self_test)
