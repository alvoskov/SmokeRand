/**
 * @file ara64.c
 * @brief ara64 (add, bit rotate, add) pseudorandom number generator;
 * it is a modification of ara32 from PractRand 0.94. It has no lower
 * boundary on its period and fails mod3 test (but passes the vast majorit
 * of other statistical tests). Useful for checking mod3 test.
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
 * 64-bit modification:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief ara64 (add, bit rotate, add) generator state
 */
typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} Ara64State;

static inline uint64_t get_bits_raw(Ara64State *obj)
{
    obj->a += rotl64(obj->b + obj->c, 13);
    obj->b += rotl64(obj->c + obj->a, 23);
    obj->c += rotl64(obj->a + obj->b, 31);
    return obj->a;
}


static void *create(const CallerAPI *intf)
{
    Ara64State *obj = intf->malloc(sizeof(Ara64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64() | 0x1;
    return obj;
}

MAKE_UINT64_PRNG("ara64", NULL)
