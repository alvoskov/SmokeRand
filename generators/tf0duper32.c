#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Tf0Duper32 PRNG state
 */
typedef struct {
    uint32_t tf;
    uint32_t xs;
} Tf0Duper32State;


static inline uint64_t get_bits_raw(Tf0Duper32State *obj)
{
    const uint32_t s = rotl32(obj->tf, 5) + obj->xs;
    obj->tf += obj->tf * obj->tf | 0x4005;
    obj->xs ^= obj->xs << 1;
    obj->xs ^= rotl32(obj->xs, 9) ^ rotl32(obj->xs, 27);
    return s ^ (s >> 16); // To prevent PractRand failure at 8 TiB
}


static void *create(const CallerAPI *intf)
{
    Tf0Duper32State *obj = intf->malloc(sizeof(Tf0Duper32State));
    const uint64_t seed = intf->get_seed64();
    obj->tf = (uint32_t) (seed & 0xFFFFFFFF);
    obj->xs = (uint32_t) (seed >> 32);
    if (obj->xs == 0) {
        obj->xs = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT32_PRNG("Tf0Duper32", NULL)
