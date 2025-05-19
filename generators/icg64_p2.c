/**
 * @file icg64_p2.c
 * @brief Inversive congruential generator with power of 2 modulus
 * @details 
 *
 * References:
 *
 * 1. https://lemire.me/blog/2017/09/18/computing-the-inverse-of-odd-integers/
 * 2. https://arxiv.org/pdf/1209.6626v2
 * 3. https://www.jstor.org/stable/1403647
 *
 *     x = 12345
 *     for i in range(0,10000):
 *         x = (6906969069 * pow(x, -1, 2**64) + 1234513250) % 2**64
 *     print(hex(x))
 *
 * Fails the `bspace8_8d`, `bspace4_8d_dec`, `bspace4_16d` tests.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
} Icg64State;

static inline uint64_t f64(uint64_t x, uint64_t y)
{
    return y * (2 - y * x); 
}

static uint64_t modinv64_p2(uint64_t x)
{
    uint64_t y = (3 * x) ^ 2; // 5 bits
    y = f64(x, y); // 10 bits
    y = f64(x, y); // 20 bits
    y = f64(x, y); // 40 bits
    y = f64(x, y); // 80 bits
    return y;
}

static inline uint64_t get_bits_raw(void *state)
{
    Icg64State *obj = state;
    obj->x = 6906969069ull * modinv64_p2(obj->x) + 1234513250ull;
    return obj->x >> 32;
}

static void *create(const CallerAPI *intf)
{
    Icg64State *obj = intf->malloc(sizeof(Icg64State));
    obj->x = intf->get_seed64() | 0x1;
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg64State obj = {.x = 12345};
    uint32_t u, u_ref = 0x7def6e56;
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("ICG64_P2", run_self_test)
