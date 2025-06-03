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
} Mwc8State;

static inline uint8_t get_bits8(Mwc8State *obj)
{
    static const uint16_t a = 108;
    obj->pos++;
    uint16_t p = a * (uint16_t) (obj->x[(obj->pos - 15) & 0xF]) + (uint16_t) obj->c;
    uint8_t x = (uint8_t) p;
    obj->x[obj->pos & 0xF] = x;
    obj->c = p >> 8;
    x = (x << 5) | (x >> 3);
    return (x ^ obj->x[(obj->pos - 1) & 0xF]) + obj->x[(obj->pos - 2) & 0xF];
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

static void Mwc8State_init(Mwc8State *obj, uint32_t seed)
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
    Mwc8State *obj = intf->malloc(sizeof(Mwc8State));
    Mwc8State_init(obj, intf->get_seed32());
/*
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 16; j++) {
            intf->printf("%.2X ", (int) obj->x[j]);
        }
        intf->printf(" : %X ", (int) obj->c);
        intf->printf("| %X \n ", (int) get_bits_raw(obj));
    }
*/
    return obj;
}

MAKE_UINT32_PRNG("Alfib8x5", NULL)
