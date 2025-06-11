/**
 * @file sapparot.c
 * @brief Sapparot is a chaotic generator.
 * @details References:
 *
 * 1.Ilya O. Levin. Sapparot. http://www.literatecode.com/sapparot
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
    uint32_t a;
    uint32_t b;
} SapparotState;

static inline uint64_t get_bits_raw(void *state)
{
    SapparotState *obj = state;
    obj->a += 0x9e3779b9;
    obj->a = (obj->a << 7) | (obj->a >> 25);
    obj->b ^= (~obj->a) ^ (obj->a << 3);
    obj->b = (obj->b << 7) | (obj->b >> 25);
    uint32_t t = obj->a;
    obj->a = obj->b;
    obj->b = t;
    return obj->a ^ obj->b;
}

static void *create(const CallerAPI *intf)
{
    SapparotState *obj = intf->malloc(sizeof(SapparotState));
    uint64_t seed = intf->get_seed64();    
    obj->a = seed >> 32;
    obj->b = seed & 0xFFFFFFFF;
    return (void *) obj;
}

static int run_self_test(const CallerAPI *intf)
{
    SapparotState obj = {.a = 0x9E3779B9, .b = 0x12345678};
    static const uint32_t x_ref[8] = {
        0x88958DAE, 0xE5BCAF84, 0xA91FDAD0, 0x50667BB5,
        0x0A4F5CB0, 0xDEF039B0, 0xF21A594B, 0x1799BECA
    };
    int is_ok = 1;
    intf->printf("%8s | %8s\n", "Out", "Ref");
    for (int i = 0; i < 8; i++) {
        uint32_t u = (uint32_t) get_bits_raw(&obj), u_ref = x_ref[i];
        intf->printf("%.8X | %.8X\n", u, u_ref);
        if (u != u_ref) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}

MAKE_UINT32_PRNG("sapparot", run_self_test)
