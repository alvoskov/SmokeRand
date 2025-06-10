/**
 * @file mwc192.c
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
    uint64_t x;
    uint64_t x2;
    uint64_t c;
} MWC192State;


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
//    static const uint64_t MWC_A1 = 0xf21f494c589bf715;
//    static const uint64_t MWC_A1 = 0x8F1;
    static const uint64_t MWC_A1 = 0x621;

    static const uint64_t LCG_A1 = 18000690696906969069ULL;
    MWC192State *obj = state;
    uint64_t out = (LCG_A1 * obj->x) ^ rotl64(obj->x2, 11); // >= 2 TiB
    //uint64_t out = (obj->x) ^ rotl64(obj->x2, 11); // 32 GiB (without scramblers - 16 MiB)
    uint64_t x_new = unsigned_muladd128(MWC_A1, obj->x2, obj->c, &obj->c);
    obj->x2 = obj->x;
    obj->x = x_new;
    return out;
}

static void *create(const CallerAPI *intf)
{
    MWC192State *obj = intf->malloc(sizeof(MWC192State));
    obj->x = intf->get_seed64();
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


MAKE_UINT64_PRNG("MWC192", run_self_test)
