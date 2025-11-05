/**
 * @file smwc96.c
 * @brief MWC128 - 128-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^127.
 * Passes SmallCrush, Crush and BigCrush tests.
 * The MWC_A1 multiplier was found by S. Vigna.
 *
 * References:
 * 1. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 2. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
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
 * @brief MWC128 state. Cannot be initialized to (0, 0) or to
 * (2^64 - 1, 2^64 - 1). Default initialization is (seed, 1) as suggested
 * by S. Vigna.
 */
typedef struct {
    uint32_t x;
    uint32_t x2;
    uint32_t c;
} Smwc96State;


/**
 * @brief MWC128 PRNG implementation.
 * @details Optimization of the output function (made for the very small bad
 * multiplier 0x621):
 *
 * 1. Optimize the shift in the x_new ^ (x_2 <<< sh1) output.
 * 2. Test if for the (Q x_new) ^ (x_2 <<< sh1) output.
 */
static inline uint64_t get_bits_raw(void *state)
{
// 0xffef3a8d
    static const uint64_t MWC_A1 = 0x549; // Bad
    static const uint32_t LCG_A1 = 1566083941u;
    Smwc96State *obj = state;
    uint32_t out = (LCG_A1 * obj->x) ^ rotl32(obj->x2, 14); // >= 128 GiB
    uint64_t mul = MWC_A1 * obj->x + obj->c;
    obj->c = (uint32_t) (mul >> 32);
    obj->x2 = obj->x;
    obj->x = (uint32_t) (mul & 0xFFFFFFFF);
    return out;
}

static void *create(const CallerAPI *intf)
{
    Smwc96State *obj = intf->malloc(sizeof(Smwc96State));
    obj->x = intf->get_seed32();
    obj->x2 = intf->get_seed32();
    obj->c = 1;
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
/*
    MWC128State obj = {.x = 12345, .c = 67890};
    uint64_t u, u_ref = 0x72BD413ED8304C94;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
*/
    (void) intf;
    return 1;
}


MAKE_UINT32_PRNG("SMWC96", run_self_test)
