/**
 * @file mwc16.c
 * @brief A simple multiply-with carry generator for 16-bit systems.
 * @details May be useful for retrocomputing. Passes the `express`, `brief`,
 * `default`, `full` batteries.
 * 
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t x[8];
    uint16_t c;
    uint8_t pos;
} Mwc16State;

static inline uint16_t rotl16(uint16_t x, int r)
{
    return (x << r) | (x >> (16 - r));
}

static inline uint16_t get_bits16(Mwc16State *obj)
{
    static const uint32_t a = 59814;
    uint16_t x_prev = obj->x[obj->pos++];
    obj->pos &= 0x7;
    uint32_t p = a * (uint32_t) (obj->x[obj->pos]) + obj->c;
    uint16_t x = (uint16_t) p;
    obj->x[obj->pos] = x;
    obj->c = p >> 16;
    // Scrambler: tested with the 123 bad multiplier
    return (x ^ rotl16(x_prev, 3)) + rotl16(x_prev, 9);
}


static inline uint64_t get_bits_raw(void *state)
{
    Mwc16State *obj = state;
    uint32_t hi = get_bits16(obj);
    uint32_t lo = get_bits16(obj);
    return (hi << 16) | lo;
}

static void Mwc16State_init(Mwc16State *obj, uint64_t seed)
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
    Mwc16State *obj = intf->malloc(sizeof(Mwc16State));
    Mwc16State_init(obj, intf->get_seed64());
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

MAKE_UINT32_PRNG("Mwc16", NULL)
