/**
 * @file seed.c
 * @brief An implementation of SHR3 - classic 32-bit LSFR generator
 * proposed by G. Marsaglia.
 * @details Fails almost all statistical tests. Note: some versions of
 * SHR3 contain a typo and use [17,13,5] instead of [13,17,5].
 *
 * References:
 *
 * - https://eprint.iacr.org/2011/007.pdf
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Keeps pointer to API with seeder.
 */
typedef struct {
    const CallerAPI *intf;
} SeedState;


static inline uint64_t get_bits_raw(void *state)
{
    SeedState *obj = state;
    return obj->intf->get_seed64();
}


static void *create(const CallerAPI *intf)
{
    SeedState *obj = intf->malloc(sizeof(SeedState));
    obj->intf = intf;
    return obj;
}


MAKE_UINT64_PRNG("Seed", NULL)
