/**
 * @file icg64.c
 * @brief Inversive congruential generator with prime modulus.
 * @details
 *
 *     x = 12345
 *     for i in range(0,10000):
 *         x = (pow(x, -1, 2**63 - 25) + 1) % (2**63 - 25)
 *     print(hex(x * 2))
 *
 * https://doi.org/10.1007/b97644 (algorithm 2.20)
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    ICG64_MOD = 0x7fffffffffffffe7 // 2^63 - 25
};

typedef struct {
    uint64_t x;
} Icg64State;

/*
 * Compute the inverse of x mod M by the modified Euclide
 * algorithm (Knuth V2 p. 325).
 */
int64_t modinv64(int64_t M, int64_t x)
{
    int64_t u1 = 0, u3 = M, v1 = 1, v3 = x;
    if (x == 0) return 0;

    while (v3 != 0) {
        int64_t qq = u3 / v3;
        int64_t t1 = u1 - v1 * qq;
        int64_t t3 = u3 - v3 * qq;
        u1 = v1;
        v1 = t1;
        u3 = v3;
        v3 = t3;
    }
    if (u1 < 0)
        u1 += M;
    return u1;
}

static inline uint64_t get_bits_raw(void *state)
{
    Icg64State *obj = state;
    obj->x = (modinv64(ICG64_MOD, obj->x) + 1) % ICG64_MOD;
    return obj->x >> 31;
}

static void *create(const CallerAPI *intf)
{
    Icg64State *obj = intf->malloc(sizeof(Icg64State));
    obj->x = intf->get_seed64() % ICG64_MOD;
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    Icg64State obj = {.x = 12345};
    uint32_t u, u_ref = 0xe5a6beea;
    for (int i = 0; i < 10000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("ICG64", run_self_test)
