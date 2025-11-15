/**
 * @file xsh.c
 * @brief An implementation of 64-bit LSFR generator proposed by G. Marsaglia.
 * @details This version of "xorshift" generator was included in KISS64 PRNG.
 * Fails linearcomp, matrixrank and some bspace and collover tests.
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright xorshift64 algorithm was suggested by G. Marsaglia.
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
 * @brief XSH PRNG state
 */
typedef struct {
    uint64_t x;
} XSHState;


static inline uint64_t get_bits_raw(void *state)
{
    XSHState *obj = state;
    obj->x ^= (obj->x << 13);
    obj->x ^= (obj->x >> 17);
    obj->x ^= (obj->x << 43);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    XSHState *obj = intf->malloc(sizeof(XSHState));
    obj->x = intf->get_seed64();
    if (obj->x == 0) {
        obj->x = 0x12345678;
    }
    return obj;
}


MAKE_UINT64_PRNG("XSH", NULL)
