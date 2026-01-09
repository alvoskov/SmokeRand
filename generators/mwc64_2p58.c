/**
 * @file mwc64_2p58.c
 * @brief 64-bit multiply-with-carry PRNG taken from KISS64 generator
 * by George Marsaglia.
 * @details It is developed by George Marsaglia. References:
 *
 * - https://groups.google.com/g/comp.lang.fortran/c/qFv18ql_WlU
 * - https://www.thecodingforums.com/threads/64-bit-kiss-rngs.673657/
 * - https://ssau.ru/pagefiles/sbornik_pit_2021.pdf
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 *
 * KISSS64 algorithm was developed by George Marsaglia.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;  ///< MWC state 1
    uint64_t c;  ///< MWC state 2
} Mwc64State;


static inline uint64_t get_bits_raw(Mwc64State *obj)
{
    const uint64_t t = (obj->x << 58) + obj->c;
    obj->c = obj->x >> 6;
    obj->x += t;
    obj->c += (obj->x < t);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Mwc64State *obj = intf->malloc(sizeof(Mwc64State));
    do {obj->x = intf->get_seed64(); } while (obj->x == 0);    
    obj->c = intf->get_seed64() >> 16;
    return obj;
}

MAKE_UINT64_PRNG("Mwc64_2p58", NULL)
