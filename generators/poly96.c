/**
 * @file poly96.c
 * @brief poly96 is a LFSR, its period is \f$2^{96} - 1\f$
 * @details
 *
 * References:
 *
 * 1. P. L'Ecuyer and F. Panneton. A new class of linear feedback shift
 *    register generators. 2000 Winter Simulation Conference Proceedings
 *    (Cat. No.00CH37165), Orlando, FL, USA, 2000, pp. 690-696 vol.1
 *    https://doi.org/10.1109/WSC.2000.899781
 *
 * @copyright
 * poly96 PRNG was designed by P. L'Ecuyer and F. Panneton.
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
    uint32_t s[3];
} Poly96State;

static inline uint64_t get_bits_raw(Poly96State *obj)
{
    static const uint32_t a0  = 0x4b24716e, a1  = 0xfbc6cd96, a2  = 0x0ab7ab0c;
    static const uint32_t b10 = 0x2fa51fb4, b11 = 0x2e1e2000, b12 = 0x03000000;
    static const uint32_t b20 = 0x78d849e0;// b21 = 0x55db0000;
    // Bitwise rotation by 71 bits to the left + elimination of bit 84
    const uint32_t w0 = (obj->s[0] >> 25) ^ (obj->s[2] << 7);
    const uint32_t w1 = (obj->s[1] >> 25) ^ (obj->s[0] << 7);
    const uint32_t w2 = ((obj->s[2] >> 25) ^ (obj->s[1] << 7)) & 0xfffff7ff;
    // Verification of bit S9
    if (obj->s[1] & 0x10) {
        obj->s[0] = w0 ^ a0;
        obj->s[1] = w1 ^ a1;
        obj->s[2] = w2 ^ a2;
    } else {
        obj->s[0] = w0;
        obj->s[1] = w1;
        obj->s[2] = w2;
    }
    // Self-temperingwith c=32 and d=10
    const uint32_t e = (obj->s[0] ^ obj->s[1] ^ obj->s[2]) << 10;
    uint32_t y0 = obj->s[0] ^ e;
    uint32_t y1 = obj->s[1] ^ e;
    uint32_t y2 = obj->s[2] ^ e;
    // MK-tempering with s1=23 and s2=47
    y0 ^= ((y1 >> 9) ^ (y0 << 23)) & b10;
    y1 ^= ((y2 >> 9) ^ (y1 << 23)) & b11;
    y2 ^= (y2 << 23) & b12;
    y0 ^= ((y2 >> 17) ^ (y1 << 15)) & b20;
    //y1 ^= (y2 << 15) & b21;
    return y0;
}


static void *create(const CallerAPI *intf)
{
    Poly96State *obj = intf->malloc(sizeof(Poly96State));
    seeds_to_array_u32(intf, obj->s, 3);
    if (obj->s[0] == 0 && obj->s[1] == 0 && obj->s[2] == 0) {
        obj->s[0] = 0x12345678;
        obj->s[1] = 0x87654321;
        obj->s[2] = 0xABCDEF;
    }
    return obj;
}


MAKE_UINT32_PRNG("poly96", NULL)
