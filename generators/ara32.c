/**
 * @file ara32.c
 * @brief ara32 (add, bit rotate, add) pseudorandom number generator
 * from PractRand 0.94. It has no lower boundary on its period and
 * fails mod3 test (but passes the vast majority of other statistical
 * tests). Useful for checking mod3 test.
 *
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 *
 * The generator was added mainly for testing the `mod3` test: it is the
 * only test that it fails.
 *
 * @copyright The ara32 algorithm was found in the sources of PractRand 0.94
 * that was developed by Chris Doty-Humphrey.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief ara32 (add, bit rotate, add) generator state
 */
typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
} Ara32State;

static inline uint64_t get_bits_raw(void *state)
{
    Ara32State *obj = state;
    obj->a += rotl32(obj->b + obj->c, 7);
    obj->b += rotl32(obj->c + obj->a, 11);
    obj->c += rotl32(obj->a + obj->b, 15);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Ara32State *obj = intf->malloc(sizeof(Ara32State));
    obj->a = intf->get_seed32();
    obj->b = intf->get_seed32();
    obj->c = intf->get_seed32() | 0x1;
    return (void *) obj;
}

MAKE_UINT32_PRNG("ara32", NULL)
