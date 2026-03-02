/**
 * @file ranlim32.c
 * @brief ranlim32 is a modification of KISS99 from Numerical Recipes
 * (3rd edition).
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t lcg;
    uint32_t xs;
    uint32_t mwc1;
    uint32_t mwc2;
} Ranlim32State;


static uint64_t get_bits_raw(Ranlim32State *obj)
{
    // 32-bit LCG with a good multiplier
    // Merit8 = 0.7546, Merit8_H = 0.7821
    // mu(2..8) = [2.0659; 3.0707; 4.0063; 5.4059; 6.3384; 8.9546]
    obj->lcg = 2891336453U * obj->lcg + 1640531513U;
    // xorshift32 (differs from Marsaglia shr3 but still has a full period)
    obj->xs ^= obj->xs >> 13;
    obj->xs ^= obj->xs << 17;
    obj->xs ^= obj->xs >> 5;
    // MWC part (multipliers are OK)
    obj->mwc1 = 33378U * (obj->mwc1 & 0xFFFFU) + (obj->mwc1 >> 16);
    obj->mwc2 = 57225U * (obj->mwc2 & 0xFFFFU) + (obj->mwc2 >> 16);
    // Scramblers
    uint32_t x = obj->lcg ^ (obj->lcg << 9);
    x ^= x >> 17;
    x ^= x << 6;
    uint32_t y = obj->mwc1 ^ (obj->mwc1 << 17);
    y ^= y >> 15;
    y ^= y << 5;
    return (x + obj->xs) ^ (y + obj->mwc2);
}

static void *create(const CallerAPI *intf)
{
    Ranlim32State *obj = intf->malloc(sizeof(Ranlim32State));
    const uint64_t seed0 = intf->get_seed64(); // For MWC
    const uint64_t seed1 = intf->get_seed64(); // For xorshift32 and LCG
    obj->mwc1 = (uint32_t) (seed0 & 0xFFFF) | 0x10000; // MWC generator 1: prevent bad seeds
    obj->mwc2 = (uint32_t) ((seed0 >> 16) & 0xFFFF) | 0x10000; // MWC generator 2: prevent bad seeds
    obj->xs   = (uint32_t) (seed1 >> 32) | 0x1; // SHR3 mustn't be init with 0
    obj->lcg  = (uint32_t) seed1; // LCG accepts any seed
    return obj;
}

MAKE_UINT32_PRNG("Ranlim32", NULL)
