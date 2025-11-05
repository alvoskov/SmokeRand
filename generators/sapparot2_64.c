/**
 * @file sapparot2_64.c
 * @brief Sapparot2 is a chaotic generator.
 * @details References:
 *
 * 1. Ilya O. Levin. Sapparot-2 Fast Pseudo-Random Number Generator.
 *    http://www.literatecode.com/sapparot2
 *
 * WARNING! IT HAS NO GUARANTEED MINIMAL PERIOD! BAD SEEDS ARE POSSIBLE!
 * DON'T USE THIS PRNG FOR ANY SERIOUS WORK! The author uses rotations
 * in the "discrete Weyl sequence" part without proof of a minimal cycle
 * of an updated counter part.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} Sapparot2x64State;


enum {
    PHI = 0x9E3779B97F4A7C55ULL,
    C_RTR = 13,
    C_SH = 58
};

static inline uint64_t get_bits_raw(void *state)
{
    uint64_t m;
    Sapparot2x64State *obj = state;
    obj->c += obj->a;
    obj->c = rotl64(obj->c, (int) (obj->b >> C_SH));
    obj->b = (obj->b + ((obj->a << 1) + 1)) ^ rotl64(obj->b, 5);
    obj->a += PHI;
    obj->a = rotl64(obj->a, C_RTR);
    m = obj->a;
    obj->a = obj->b;
    obj->b = m;
    return obj->c ^ obj->b ^ obj->a;
}

static void *create(const CallerAPI *intf)
{
    Sapparot2x64State *obj = intf->malloc(sizeof(Sapparot2x64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    const uint64_t u_ref = 0x3FCF27C391F28B45;
    uint64_t u;
    Sapparot2x64State obj = {.a = 0, .b = 0, .c = 0};
    for (int i = 0; i < 10000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: %llX, reference: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("sapparot2_64", run_self_test)
