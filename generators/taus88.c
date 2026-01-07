/**
 * @file taus88.c
 * @brief A combination of three LFSRs.
 * @details References:
 *
 * 1. Pierre L'Ecuyer Maximally equidistributed combined Tausworthe generators
 *    // Mathematics of Computation. 1996. V. 65. N 213. P.203-213.
 *    https://doi.org/10.1090/S0025-5718-96-00696-5
 * 2. ISO 28640:2010. Random variate generation methods (IDT)
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t s[3];
} Taus88State;


void Taus88State_init(Taus88State *obj, uint32_t s)
{
    for (int i = 0; i < 3; ) {
        if (s & 0xFFFFFFF0U) {
            obj->s[i] = s;
            i++;
        }
        s = 1664525U * s + 1U;
    }
}

void *create(const CallerAPI *intf)
{
    Taus88State *obj = intf->malloc(sizeof(Taus88State));
    Taus88State_init(obj, intf->get_seed32());
    return obj;
}

uint64_t get_bits_raw(Taus88State *obj)
{
    uint32_t s1 = obj->s[0], s2 = obj->s[1], s3 = obj->s[2], b;
    b  = (((s1 << 13) ^ s1) >> 19);
    s1 = (((s1 & 0xFFFFFFFEU) << 12) ^ b);
    b  = (((s2 << 2) ^ s2) >> 25);
    s2 = (((s2 & 0xFFFFFFF8U) << 4) ^ b);
    b  = (((s3 << 3 ) ^ s3) >> 11);
    s3 = (((s3 & 0xFFFFFFF0U) << 17 ) ^ b);
    obj->s[0] = s1; obj->s[1] = s2; obj->s[2] = s3;
    return (s1 ^ s2 ^ s3);
}

/**
 * @brief An internal self-test based on reference values from ISO 28640:2010
 * "Random variate generation methods (IDT)"
 */
static int run_self_test(const CallerAPI *intf)
{
    Taus88State obj;
    Taus88State_init(&obj, 19660809);
    uint32_t u, u_ref = 262361229;
    for (int i = 0; i < 5000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    u >>= 1;
    intf->printf("%u %u\n", u, u_ref);
    return (u == u_ref);
}

MAKE_UINT32_PRNG("Taus88", run_self_test)
