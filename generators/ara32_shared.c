/**
 * @file ara32_shared.c
 * @brief ara32 (add, bit rotate, add) pseudorandom number generator
 * from PractRand 0.94. It has no lower boundary on its period and
 * fails mod3 test (but passes the vast majority of other statistical
 * tests). Useful for checking mod3 test.
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

static inline uint32_t rotl32(uint32_t x, unsigned int r)
{
    return (x << r) | (x >> (32 - r));
}

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
