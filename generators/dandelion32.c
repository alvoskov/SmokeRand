/**
 * @file dandelion32.c
 * @brief dandelion32 is a xorshift-like LFSR that uses arithmetic right shift
 * and a non-bijective output scrambler.
 * @details This PRNG was designed by Aaron Pribadi. It uses the next
 * transition function and the next output function `F(x, y)`:
 *
 * - `T(x, y) = (y ^ lsl(x, sh1), x ^ asr(y, sh2)`
 * - `F(x, y) = ((x*x) % 2**32) + y) ^ ((x*x) // 2**32)`
 *
 * It has a proven period of \f$ 2^{32} - 1 \f$. Note: it also uses
 * an implementation defined behaviour of the `>>` operator for the signed
 * integers, but it is almost always implemented as an arithmetic shift.
 *
 * References:
 *
 * - https://github.com/apribadi/dandelion
 *
 * About shifts: `(5, 3)` or `(7, 6)` pairs are possible, but the first one
 * is better from the viewpoint of Hamming weights distribution
 * (`hamming_distr` test).
 *
 * WARNING! The period is too small for any serious use. So its made just as
 * scaled down "toy" version of dandelion64/128.
 *
 * @copyright The dandelion PRNG was developed by Aaron Pribadi. The
 * parameters for the "toy" 16-bit version were found by A.L. Voskov
 *
 * Reentrant portable C99 implementation:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Dandelion64 PRNG state.
 */
typedef struct {
    uint16_t x;
    uint16_t y;
} Dandelion32State;


static inline uint16_t get_bits16_raw(Dandelion32State *obj)
{
    const uint16_t x = obj->x, y = obj->y;
    obj->x = (uint16_t) (y ^ (x << 5));
    obj->y = (uint16_t) (x ^ (uint16_t) ((int16_t) y >> 3));
    const uint32_t xsq = (uint32_t) x * (uint32_t) x;
    const uint16_t hi = (uint16_t) (xsq >> 16), lo = (uint16_t) xsq;
    return (uint16_t) ((lo + y) ^ hi);
}


static inline uint64_t get_bits_raw(Dandelion32State *obj)
{
    const uint32_t hi = get_bits16_raw(obj);
    const uint32_t lo = get_bits16_raw(obj);
    return (hi << 16) | lo;
}

static void *create(const CallerAPI *intf)
{
    Dandelion32State *obj = intf->malloc(sizeof(Dandelion32State));
    const uint64_t s = intf->get_seed64();
    obj->x = (uint16_t) s;
    obj->y = (uint16_t) (s >> 16);
    if (obj->x == 0 && obj->y == 0) {
        obj->x = 0xDEAD;
        obj->y = 0x1234;
    }
    return obj;    
}

MAKE_UINT32_PRNG("dandelion32", NULL)
