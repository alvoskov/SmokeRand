/**
 * @file mwc8.c 
 * @brief
 * @details
from sympy import *
n = 15
b = 2**8
for i in range(1, 255):
    a = b - i
    p = a * b ** n - 1
    p2 = (p - 1) // 2
    if isprime(p):# and isprime((i * 2**(8*n) - 2) / 2):
        print(a, i, hex(a), isprime(p2))

 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint8_t x[16];
    uint8_t c;
    uint8_t pos;
} Smwc8x16State;

static inline uint8_t get_bits8(Smwc8x16State *obj)
{
    static const uint16_t a = 108u;
    static const uint8_t a_lcg = 137u;
    obj->pos++;
    uint16_t p = a * (uint16_t) (obj->x[(obj->pos - 15) & 0xF]) + (uint16_t) obj->c;
    uint8_t x = (uint8_t) p;
    obj->x[obj->pos & 0xF] = x;
    obj->c = p >> 8;
    uint8_t x_prev = obj->x[(obj->pos - 1) & 0xF];
    return ((a_lcg * x) ^ ((x_prev << 3) | (x_prev >> 5))) + obj->x[(obj->pos - 2) & 0xF];
//    x = (x << 5) | (x >> 3);
//    return (x ^ obj->x[(obj->pos - 1) & 0xF]) + obj->x[(obj->pos - 2) & 0xF];
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = get_bits8(state);
    }
    return out.u32;    
}

static void Smwc8x16State_init(Smwc8x16State *obj, uint32_t seed)
{
    obj->c = 1;
    for (int i = 0; i < 16; i++) {
        int sh = (i % 4) * 8;
        obj->x[i] = (uint8_t) ((seed >> sh) + i);
    }
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Smwc8x16State *obj = intf->malloc(sizeof(Smwc8x16State));
    Smwc8x16State_init(obj, intf->get_seed32());
    return obj;
}

MAKE_UINT32_PRNG("Smwc8x16", NULL)
