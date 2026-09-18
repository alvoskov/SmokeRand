/**
 * @file xorshift320.c
 * @brief An implementation of 320-bit LSFR generator proposed by G. Marsaglia.
 * @details The recommended shifts triple is `[31 1 28]` (passes `express`,
 * `brief` and `default`, suspicious values for `hamming_ot_u128` in the `full`
 * battery). Fails BCFN test from PractRand 0.96 at 32 GiB.
 *
 * Other less optimal triples:
 * 
 * - `[9 3 2]` fails `express` (birthday spacings)
 * - `[10 1 32]`, `[10 13 2]`, `[10 13 2]`, `[9 3 25]`, `[7 3 34]`, `[7 35 1]`:
 *   fails maxoft tests from the `brief` battery.
 * - `[16 5 52]`: passes `brief`, fails maxoft from `full` battery.
 * - `[24 5 3]` fails `bspace8_8d` from `brief`
 * - `[31 1 28]`: +brief/+default/full:suspicious hammming_ot_u128
 * - `[37 1 30]`: +brief/ but suspicious HWs from full
 * - `[41 3 6]`:  +brief/ but suspicious HWs from full
 *
 * References:
 * 
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 *
 * @copyright The xorshift algorithm was suggested by G. Marsaglia.
 *
 * xorshift320 parameters and its implementation:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift320 PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
    uint64_t v;
} Xorshift320State;


static inline uint64_t get_bits_raw(Xorshift320State *obj)
{
    const int a = 31, b = 1, c = 28;
    uint64_t t = obj->x ^ (obj->x << a); // a
    t ^= t >> b; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> c)) ^ t; // c
    return obj->z;
}


static void *create(const CallerAPI *intf)
{
    Xorshift320State *obj = intf->malloc(sizeof(Xorshift320State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    obj->v = intf->get_seed64();
    if (obj->v == 0) { // State mustn't be all zeros
        obj->v = 0xDEADBEEF;
    }        
    return obj;
}

MAKE_UINT64_PRNG("Xorshift320", NULL)
