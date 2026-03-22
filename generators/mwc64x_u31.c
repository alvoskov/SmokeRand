/**
 * @file mwc64x_u31.c
 * @brief MWC64X - a 31-bit modification of the MWC64X generator.
 * @details Uses the `(x ^ c) << 1` output function instead of `(x ^ c)`.
 * Designed for testing the `--filter=uint31` mode, passes PractRand at least
 * up to 8 TiB.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC64X state.
 */
typedef struct {
    uint64_t data;
} MWC64XState;

static inline uint64_t get_bits_raw(MWC64XState *obj)
{
    const uint64_t A0 = 0xff676488; // 2^32 - 10001272
    uint32_t c = (uint32_t) (obj->data >> 32);
    uint32_t x = (uint32_t) (obj->data & 0xFFFFFFFF);
    obj->data = A0 * x + c;
    const uint32_t out = (x ^ c) << 1;
    return out;
}

static void *create(const CallerAPI *intf)
{
    MWC64XState *obj = intf->malloc(sizeof(MWC64XState));
    // Seeding: prevent (0,0) and (?,0xFFFF)
    do {
        obj->data = intf->get_seed64() << 1;
    } while (obj->data == 0);
    return obj;
}

MAKE_UINT32_PRNG("MWC64X_U31", NULL)
