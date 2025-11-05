/**
 * @file swbmwc32.c
 * @brief A combined 32-bit generator made from subtract-with-borrow (SWB) and
 * multiply-with-carry PRNG.
 * @details Taken from DIEHARD test suite. It fails the `gap_inv8` test from
 * `brief`, `default` and `full` batteries that makes it unsuitable as a general
 * purpose pseudorandom number generator.
 *
 * 1. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright The SWB algorithm was suggested by G.Marsaglia and A.Zaman.
 *
 * Implementation for SmokeRand:
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 37
#define SWB_B 24

/**
 * @brief 32-bit SWBMWC state.
 */
typedef struct {    
    uint32_t x[SWB_A];
    uint32_t c;
    uint32_t mwc;
    int i;
    int j;
} SwbMwc32State;


static inline uint64_t get_bits_raw(void *state)
{
    static const uint32_t MWC_A = 30903;
    SwbMwc32State *obj = state;
    // SWB part
    const uint32_t xj = obj->x[obj->j], xi = obj->x[obj->i];
    const uint32_t t = xj - xi - obj->c;
    obj->c = (xj < t) ? 1 : 0;
    obj->x[obj->i] = t;
    if (obj->i == 0) { obj->i = SWB_A; }
	if (obj->j == 0) { obj->j = SWB_A; }
    obj->i--;
    obj->j--;
    // MWC part
    obj->mwc = (obj->mwc & 0xFFFFu) * MWC_A + (obj->mwc >> 16);
    return t + obj->mwc;
}


static void *create(const CallerAPI *intf)
{
    SwbMwc32State *obj = intf->malloc(sizeof(SwbMwc32State));    
    for (size_t i = 0; i < SWB_A; i++) {
        obj->x[i] = (uint32_t) intf->get_seed64();
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->mwc = (uint32_t) (intf->get_seed64() & 0xFFFFFFF) | 0x1;
    obj->i = SWB_A - 1; obj->j = SWB_B - 1;
    return obj;
}


MAKE_UINT32_PRNG("SWBMWC32", NULL)
