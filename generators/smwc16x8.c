/**
 * @file mwc16x8.c
 * @brief A simple multiply-with carry generator for 16-bit systems.
 * @details May be useful for retrocomputing. Passes the `express`, `brief`,
 * `default`, `full` batteries. Uses a simple output scrambler to hide
 * possible artefacts (inspired by MWC256XXA3, tuned for a very bad
 * multiplier).
 *
 * PractRand 0.94: >= 1 TiB
 *
 * Uses the next recurrent formula for updatings its internal state:
 *
 * \f[
 * x_{i} = a x_{i - 8} + c_{i - 1} \mod 2^{16}
 * \f]
 *
 *
 * \f[
 * c_{i} = \lfloor \frac{a x_{i - 8} + c_{i - 1}}{\mod 2^{16}} \rfloor
 * \f]
 *
 * Uses the next output scrambler:
 *
 * \f[
 * u_{i} = \left(x_i \oplus (x_{i-1} \lll 3) \right) + (x_{i-1} \lll 9) \mod 2^{16}
 * \f]
 * 
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t x[8];
    uint16_t c;
    uint8_t pos;
} Smwc16x8State;

static inline uint16_t rotl16(uint16_t x, int r)
{
    return (x << r) | (x >> (16 - r));
}

static inline uint16_t get_bits16(Smwc16x8State *obj)
{
    static const uint32_t a = 59814;
    //static const uint32_t a = 1569;
    static const uint32_t a_lcg = 62517;
    uint16_t x_prev = obj->x[obj->pos++];
    obj->pos &= 0x7;
    uint32_t p = a * (uint32_t) (obj->x[obj->pos]) + obj->c;
    uint16_t x = (uint16_t) p;
    obj->x[obj->pos] = x;
    obj->c = p >> 16;
    // Scrambler: tested with the 1569 bad multiplier (1 TiB with "unusual" in BCFN)
    return (a_lcg * x) ^ rotl16(x_prev, 7);
//  Possible alternative scrambler and bad multiplier: 123
//    return (x ^ rotl16(x_prev, 3)) + rotl16(x_prev, 9);
}


static inline uint64_t get_bits_raw(void *state)
{
    Smwc16x8State *obj = state;
    uint32_t hi = get_bits16(obj);
    uint32_t lo = get_bits16(obj);
    return (hi << 16) | lo;
}

static void Smwc16x8State_init(Smwc16x8State *obj, uint64_t seed)
{
    obj->c = 1;
    for (int i = 0; i < 8; i++) {
        int sh = (i % 4) * 16;
        obj->x[i] = (uint16_t) ((seed >> sh) + i);
    }
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Smwc16x8State *obj = intf->malloc(sizeof(Smwc16x8State));
    Smwc16x8State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT32_PRNG("Smwc16x8", NULL)
