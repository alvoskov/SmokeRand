/**
 * @file mwc256.c
 * @brief MWC256 - 256-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a period about 2^255.
 * Passes SmallCrush, Crush and BigCrush tests.
 *
 * References:
 * 1. Sebastiano Vigna. MWC256. https://prng.di.unimi.it/MWC256.c
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
 * The state must be initialized so that 0 < c < MWC_A3 - 1.
 * For simplicity, we suggest to set c = 1 and x, y, z to a 192-bit seed.
 */
typedef struct {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint64_t c;
} MWC256State;

#define MWC_A3 0xfff62cf2ccc0cdaf

/**
 * @brief MWC256 PRNG implementation.
 */
static inline uint64_t get_bits_raw(MWC256State *obj)
{
	const uint64_t result = obj->z;
	// muladd128() returns the high bits, and puts the low bits in the last param
    uint64_t t            = unsigned_muladd128(MWC_A3, obj->x, obj->c, &obj->c);

	obj->x = obj->y;
	obj->y = obj->z;
	obj->z = t;
	// C is set to the lower bits in the muladd128() above

	return result;
}

static void *create(const CallerAPI *intf)
{
    MWC256State *obj = intf->malloc(sizeof(MWC256State));

    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->c = 1;

    return obj;
}

MAKE_UINT64_PRNG("MWC256", NULL)
