/**
 * @file mwc192.c
 * @brief MWC192 - 192-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^191.
 * Passes SmallCrush, Crush and BigCrush tests.
 *
 * References:
 * 1. Sebastiano Vigna. MWC192. https://prng.di.unimi.it/MWC192.c
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * The state must be initialized so that 0 < c < MWC_A2 - 1.
 * For simplicity, we suggest to set c = 1 and x, y to a 128-bit seed.
 */
typedef struct {
    uint64_t x; ///< x_{n-2}
    uint64_t y; ///< x_{n-1}
    uint64_t c; ///< carry
} MWC192State;

#define MWC_A2 0xffa04e67b3c95d86

/**
 * @brief MWC192 PRNG implementation.
 */
static inline uint64_t get_bits_raw(MWC192State *obj)
{
    const uint64_t result = obj->y;
    // muladd128() returns the high bits, and puts the low bits in the last param
    const uint64_t t      = unsigned_muladd128(MWC_A2, obj->x, obj->c, &obj->c);

    obj->x = obj->y;
    obj->y = t;
    // C is set to the lower bits in the muladd128() above

    return result;
}

static void *create(const CallerAPI *intf)
{
    MWC192State *obj = intf->malloc(sizeof(MWC192State));

    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->c = 1;

    return obj;
}

MAKE_UINT64_PRNG("MWC192", NULL)
