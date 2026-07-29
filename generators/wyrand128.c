/**
 * @file wyrand128.c
 * @brief wyrand128 resembles wyrand and is also based on a scrambled
 * "discrete Weyl sequence" but its period is 2^128.
 * @details wyrand128 was designed by Reiner Pope (https://github.com/reinerp)
 * as an extension of wyrand.
 *
 * References:
 *
 * - https://github.com/wangyi-fudan/wyhash/issues/156
 * - https://github.com/hoxxep/rapidrand/blob/reinerp-experiments/rapidrand/src/lib.rs
 *
 * @copyright wyrand128 algorithm was developed by Reiner Pope.
 *
 * Reentrant C99 implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief wyrand128 state
 */
typedef struct {
    uint64_t lo; ///< Lower part
    uint64_t hi; ///< Higher part
} WyRand128State;


static inline uint64_t get_bits_raw(WyRand128State *obj)
{
    const uint64_t inc = 0x2d358dccaa6c78a5; // Also w_hi
    const uint64_t xor = 0x8bb84b93962eacc9; // Also w_lo
    const uint64_t lo = obj->lo, hi = obj->hi;
    // Discrete Weyl sequence (counter) increment
    unsigned_add128(&obj->hi, &obj->lo, xor);
    obj->hi += inc;
    // Output function
    uint64_t p_hi, p_lo;
    p_lo = unsigned_mul128(hi, hi ^ lo, &p_hi);
    return p_lo ^ p_hi;
}


static void *create(const CallerAPI *intf)
{
    WyRand128State *obj = intf->malloc(sizeof(WyRand128State));
    obj->lo = intf->get_seed64();
    obj->hi = intf->get_seed64();
    return obj;
}

/**
 * @brief An internal self-test based on values obtained from
 * Python 3.x script.
 * @details
 *
 *    x, u = (0x12345 << 64) | 0x67890, 0
 *    inc, xor = 0x2d358dccaa6c78a5, 0x8bb84b93962eacc9
 *    weyl = (inc << 64) | xor
 *    print(hex(x), hex(weyl))
 *    for i in range(0, 100000):
 *        hi, lo = x >> 64, x % 2**64
 *        x = (x + weyl) % 2**128
 *        p = hi * (hi ^ lo)
 *        u = (p >> 64) ^ (p % 2**64)
 *    print(hex(u))
 */
static int run_self_test(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x45f1659177473575;
    WyRand128State obj = {.lo = 0x67890, .hi = 0x12345};
    for (int i = 0; i < 100000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Output: %llX; reference: %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return (u == u_ref) ? 1 : 0;
}

MAKE_UINT64_PRNG("wyrand128", run_self_test)
