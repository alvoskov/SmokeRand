/**
 * @file jlkiss64.c
 * @brief JLKISS64 pseudorandom number generator with 64-bit output.
 * @details This modification of KISS99 algorithm was developed by 
 * David Jones.
 *
 * References:
 * 1. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 * 2. https://groups.google.com/group/sci.stat.math/msg/b555f463a2959bb7/
 *
 * @copyright
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * The KISS99 algorithm is developed by George Marsaglia, its JLKISS64
 * modificaiton was suggested by David Jones.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief JLKISS64 PRNG state.
 */
typedef struct {
    uint64_t x; ///< 64-bit LCG state
    uint64_t y; ///< xorshift64 state
    uint64_t mwc1; ///< MWC 1 state
    uint64_t mwc2; ///< MWC 2 state
} JLKISS64State;


static inline uint64_t get_bits_raw(JLKISS64State *obj)
{
    // LCG part
    obj->x = 1490024343005336237ULL * obj->x + 123456789;
    // xorshift part (don't set y to 0)
    obj->y ^= obj->y << 21;
    obj->y ^= obj->y >> 17;
    obj->y ^= obj->y << 30;
    // MWC 1 part
    obj->mwc1 = 4294584393ULL * (obj->mwc1 & 0xFFFFFFFF) + (obj->mwc1 >> 32);
    obj->mwc2 = 4246477509ULL * (obj->mwc2 & 0xFFFFFFFF) + (obj->mwc2 >> 32);
    // Combined 64-bit output
    uint32_t z1 = (uint32_t) obj->mwc1;
    return obj->x + obj->y + z1 + (obj->mwc2 << 32);
}

static void *create(const CallerAPI *intf)
{
    JLKISS64State *obj = intf->malloc(sizeof(JLKISS64State));
    uint64_t s_mwc = intf->get_seed64();
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64() | 0x1;
    obj->mwc1 = (s_mwc >> 32) | (1ull << 33);
    obj->mwc2 = (s_mwc & 0xFFFFFFFF) | (1ull << 33);
    return (void *) obj;
}

MAKE_UINT64_PRNG("JLKISS64", NULL)
