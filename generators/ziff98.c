/**
 * @file ziff98.c 
 * @brief An implementation of Ziff98 LFSR.
 * @details Has much better quality than R250 or R1279 but still fails the
 * linear complexity test for all bits.
 *
 * Uses the next recurrent relationship:
 *
 * \f[
 * x_{n} = x_{n - 471} \oplus x_{n - 1586} \oplus x_{n - 6988} \oplus x_{n - 9689)
 * \f]
 *
 * where \f$ x_i \f$ are 32-bit (or n-bit) words. It was proven by the PRNG
 * author [1] that it is equivalent to the 2-tap PRNG with decimation. This
 * technique was used in RANLUX for SWB generator and in TAOCP2 for LFIB
 * generator.
 *
 * References:
 *
 * 1. Ziff R.M. Four-tap shift-register-sequence random-number generators
 *    // Computers in Physics. 1998. V. 12. N 4. P.385-392.
 *    https://doi.org/10.1063/1.168692
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    Z98_BUFSIZE = 16384,
    Z98_MASK = 0x3FFF
};

typedef struct {
    uint32_t x[Z98_BUFSIZE];
    unsigned int pos;
} Ziff98State;

static inline uint64_t get_bits_raw(void *state)
{
    Ziff98State *obj = state;
    uint32_t *x = obj->x;
    obj->pos++;
    return x[obj->pos & Z98_MASK] =
        x[(obj->pos - 471) & Z98_MASK] ^ 
        x[(obj->pos - 1586) & Z98_MASK] ^ 
        x[(obj->pos - 6988) & Z98_MASK] ^ 
        x[(obj->pos - 9689) & Z98_MASK];
}

static void Ziff98State_init(Ziff98State *obj, uint64_t seed)
{
    for (int i = 0; i < Z98_BUFSIZE; i++) {
        obj->x[i] = (uint32_t) pcg_bits64(&seed);
    }
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Ziff98State *obj = intf->malloc(sizeof(Ziff98State));
    Ziff98State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT32_PRNG("Ziff98", NULL)
