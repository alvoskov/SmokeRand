/**
 * @file loop_7fff_w64.c
 * @brief Just a loop that always returns 2^{63} - 1, the tests mustn't glitch,
 * go to infinite loops (a real possibility for gap test without extra checks).
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    (void) state;
    return (1ull << 63ull) - 1ull;
}


static void *create(const CallerAPI *intf)
{
    (void) intf;
    return intf->malloc(sizeof(uint64_t));
}

MAKE_UINT64_PRNG("loop_7fff_w64", NULL)
