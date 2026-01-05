/**
 * @file well1024a.c
 * @brief WELL1024a generator (Well equidistributed long-period linear).
 * @details This PRNG passes majority of statistical tests, fails tests based
 * on matrix ranks and linear complexity.
 *
 * References:
 *
 * 1. Panneton F., L'Ecuyer P., Matsumoto M. Improved long-period generators
 * based on linear recurrences modulo 2 // ACM Transactions on Mathematical
 * Software (TOMS). 2006. V. 32. N 1. P.1-16
 * https://doi.org/10.1145/1132973.1132974
 *
 * @copyright WELL algorithm was developed by Panneton F., L'Ecuyer P.,
 * Matsumoto M.
 * 
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


#define R 32
#define POS_MASK ( (unsigned int) (R - 1) )

typedef struct {
    uint32_t s[R];
    unsigned int pos;
} Well1024aState;

#define IND(ind) ( (obj->pos + (ind) ) & POS_MASK )

static inline uint32_t m3pos(int t, uint32_t v)
{
    return v ^ (v >> t);
}

static inline uint32_t m3neg(int t, uint32_t v)
{
    return v ^ (v << (-t));
}

static void *create(const CallerAPI *intf)
{
    Well1024aState *obj = intf->malloc(sizeof(Well1024aState));
    int is_valid = 0;
    obj->pos = 0;
    for (unsigned int i = 0; i < R; i++) {
        obj->s[i] = intf->get_seed32();
        if (obj->s[i] != 0) {
            is_valid = 1;
        }
    }
    if (!is_valid) {
        obj->s[0] = 0x9E3779B9;
    }
    return obj;
}

static inline uint64_t get_bits_raw(Well1024aState *obj)
{
    static const unsigned int m1 = 3, m2 = 24, m3 = 10;
    const unsigned int neg1ind = IND(POS_MASK);
    uint32_t *s = obj->s;
    const uint32_t z0 = s[neg1ind]; // VRm1
    const uint32_t z1 = s[obj->pos] ^ m3pos(8, s[IND(m1)]);
    const uint32_t z2 = m3neg(-19, s[IND(m2)]) ^ m3neg(-14, s[IND(m3)]);
    s[obj->pos] = z1 ^ z2; // newV1
    s[neg1ind] = m3neg(-11, z0) ^ m3neg(-7, z1) ^ m3neg(-13, z2); // newV0
    obj->pos = neg1ind;
    return s[obj->pos];
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[] = {
        0x00000081, 0x00004001, 0x00204081, 0x10000080,
        0x10000081, 0x102020C0, 0x10204000, 0x18104081,
        0x08000081, 0x10302041, 0x18283001, 0x08085081,
        0x00002001, 0x00187890, 0x0C0C58E0, 0x020868A1,
        0x061C68C1, 0x1C307C68, 0x102C10D0, 0x012E0C98,
        0x871C5C59, 0x17165C10, 0x881F4E49, 0x992752D0,
        0x59857055, 0x98AA53F6, 0x928BF714, 0x52B2D8E3,
        0xA65700BC, 0x85E02EAD, 0xF1FD6F4A, 0xAF9A8FF0}; 

    Well1024aState obj;
    int is_ok = 1;
    for (size_t i = 0; i < R; i++) {
        obj.s[i] = 0;
    }
    obj.s[0] = 1;
    obj.pos = 0;
    for (size_t i = 0; i < 32; i++) {
        uint32_t u = (uint32_t) get_bits_raw(&obj);
        if (i % 4 == 0) {
            intf->printf("\n");
        }
        intf->printf("%.8X|%.8X ", u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;

}

MAKE_UINT32_PRNG("Well1024a", run_self_test)
