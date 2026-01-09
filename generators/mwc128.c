/**
 * @file mwc128.c
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
    uint64_t c;
} MWC128State;


/**
 * @brief MWC128 PRNG implementation.
 */
static inline uint64_t get_bits_raw(MWC128State *obj)
{
    static const uint64_t MWC_A1 = 0xffebb71d94fcdaf9;
    obj->x = unsigned_muladd128(MWC_A1, obj->x, obj->c, &obj->c);
    return obj->x;
}



static void *create(const CallerAPI *intf)
{
    MWC128State *obj = intf->malloc(sizeof(MWC128State));
    obj->x = intf->get_seed64();
    obj->c = 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    MWC128State obj = {.x = 12345, .c = 67890};
    uint64_t u, u_ref = 0x72BD413ED8304C94;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("MWC128", run_self_test)
