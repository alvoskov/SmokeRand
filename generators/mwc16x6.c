#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t k;
    uint32_t j;
    uint32_t i;
    uint32_t l;
    uint32_t m;
    uint32_t n;
} Mwc16x8State;


static inline uint64_t get_bits_raw(Mwc16x8State *obj)
{
    obj->k = 30903U * (obj->k & 0xFFFFU) + (obj->k >> 16);
    obj->j = 18000U * (obj->j & 0xFFFFU) + (obj->j >> 16);
    obj->i = 29013U * (obj->i & 0xFFFFU) + (obj->i >> 16);
    obj->l = 30345U * (obj->l & 0xFFFFU) + (obj->l >> 16);
    obj->m = 30903U * (obj->m & 0xFFFFU) + (obj->m >> 16);
    obj->n = 31083U * (obj->n & 0xFFFFU) + (obj->n >> 16);
    return ((obj->k + obj->i + obj->m) >> 16) + obj->j + obj->l + obj->n;
}


static void *create(const CallerAPI *intf)
{
    const uint64_t s0 = intf->get_seed64();
    const uint64_t s1 = intf->get_seed64();
    Mwc16x8State *obj = intf->malloc(sizeof(Mwc16x8State));
    obj->k = (uint32_t) (s0 & 0xFFFFU) | 0x10000U;
    obj->j = (uint32_t) ((s0 >> 16) & 0xFFFFU) | 0x10000U;
    obj->i = (uint32_t) ((s0 >> 32) & 0xFFFFU) | 0x10000U;
    obj->l = (uint32_t) ((s0 >> 48) & 0xFFFFU) | 0x10000U;
    obj->m = (uint32_t) (s1 & 0xFFFFU) | 0x10000U;
    obj->n = (uint32_t) ((s1 >> 16) & 0xFFFFU) | 0x10000U;
    return obj;
}

MAKE_UINT32_PRNG("MWC16x6", NULL)
