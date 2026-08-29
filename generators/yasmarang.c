/**
 * @file yasmarang.c
 * @brief yasmarang is a chaotic PRNG with a short period.
 * @details This implementation was tested by means of reference vectors
 * supplied by Ilya Levin, the algorithm author. It fails a lot of tests
 * in SmokeRand, the 64-bit collision test results show that its period
 * is very short.
 *
 * References:
 *
 * 1. http://www.literatecode.com/yasmarang
 *
 * @copyright The yasmarang PRNG was developed by Ilya Levin.
 *
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t pad;
    uint32_t n;
    uint32_t d;
    uint8_t dat;    
} YasmarangState;

void YasmarangState_init(YasmarangState *obj, uint32_t seed)
{
    obj->pad = seed;
    obj->n = 69;
    obj->d = 233;
    obj->dat = 0;
}

uint64_t get_bits_raw(YasmarangState *obj)
{
    obj->pad += obj->dat + obj->d * obj->n;
    obj->pad = rotl32(obj->pad, 3);
    obj->n = obj->pad | 2;
    obj->d ^= rotl32(obj->pad, 31);
    obj->dat = (uint8_t) ( obj->dat ^ obj->pad ^ (obj->d >> 8) ^ 1);
    return (obj->pad ^ (obj->d << 5) ^ (obj->pad>>18) ^ (obj->dat << 1));
}


static void *create(const CallerAPI *intf)
{
    YasmarangState *obj = intf->malloc(sizeof(YasmarangState));
    YasmarangState_init(obj, intf->get_seed32());
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref[20] = {
        0xBF5B0806, 0x7334585E, 0xBC0C7B9D, 0xBDEC5ADE, 0xA5F12D68,
        0x978CA203, 0x8CE2E203, 0x37B2CE15, 0x1B986BD2, 0x2F3F1111,
        0x40197A2D, 0x31057AB0, 0x579401A4, 0x1CF95A3C, 0x744882AD,
        0xE6179123, 0x9E1CAC6A, 0x0D77D709, 0xFB34195F, 0x77831C40
    };
    YasmarangState obj;
    YasmarangState_init(&obj, 0xeda4baba);
    int is_ok = 1;
    intf->printf("%8s %8s\n", "Out", "Ref");
    for (int i = 0; i < 20; i++) {
        const uint32_t x = (uint32_t) get_bits_raw(&obj);
        intf->printf("%8.8lX %8.8lX\n",
            (unsigned long) x, (unsigned long) x_ref[i]);
        if (x != x_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT32_PRNG("yasmarang", run_self_test)
