/**
 * @file xorrot256mrt.c
 * @brief A scrambled version of xorrot256.
 * @details The scrambler is bijective and based on the next nonlinear
 * transformations:
 *
 * 1. Ronald L. Rivest. On the invertibility of the XOR of rotations of
 *    a binary word https://people.csail.mit.edu/rivest/pubs/Riv11e.prepub.pdf
 * 2. Klimov, A., Shamir, A. (2003) A New Class of Invertible Mappings. In:
 *    Kaliski, B.S., Koç, ç.K., Paar, C. (eds) Cryptographic Hardware and
 *    Embedded Systems - CHES 2002. CHES 2002. Lecture Notes in Computer
 *    Science, vol 2523. Springer, Berlin, Heidelberg.
 *    https://doi.org/10.1007/3-540-36400-5_34
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint64_t w;
} Xorrot256State;


static inline uint64_t get_bits_raw(Xorrot256State *obj)
{
    const uint64_t x0 = obj->x, w0 = obj->w;
    uint64_t out = rotl64(6906969069U * x0, 11);
    out += (out * out | 0x40000005);
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << 3) ^ obj->z ^ rotl64(w0, 8) ^ rotl64(w0, 37);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot256State *obj = intf->malloc(sizeof(Xorrot256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    if (obj->w == 0) {
        obj->w = 0x123456789ABCDEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("xorrot256mrt", NULL)
