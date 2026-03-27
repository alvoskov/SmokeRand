/**
 * @file mwc4159_u64.c
 * @brief A 64-bit MWC generator with large lag (32) and base 2^64 - 1.
 * Has period around 2^4159 (~2^{4158.946}).
 * @details See mwc2110_u64 source code for Python 3.x script for multipliers
 * search!
 *
 * TODO: NOT DONE YET!
 * Note: both \f$ m \f$ and all \f$ m - 1 \f$ cofactors were proven to be prime
 * by means of Primo 4.3.3, see the misc/mwc2110_cert directory.
 *
 * Possible candidate for lag32: 15749752082985278118 0xda925c12dece5aa6
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

#define MWC4159_LAG 64

/**
 * @brief MWC4159-u64 PRNG state.
 */
typedef struct {
    uint64_t x[MWC4159_LAG]; ///< Generated values
    uint64_t c; ///< Carry
    unsigned int pos; ///< Current position in the buffer
} Mwc4159u64State;


static inline void Mwc4159u64State_fill_buffer(Mwc4159u64State *obj)
{
    static const uint64_t MWC_A = 0xf6a64c36f2d94699U;
    for (int i = 0; i < MWC4159_LAG; i++) {
        obj->x[i] = unsigned_muladd128(MWC_A, obj->x[i], obj->c, &obj->c);
    }
}


static inline uint64_t tf0sc_next(uint64_t *state)
{
    *state += *state * *state | 0x40000005;
    uint64_t u = *state ^ (*state >> 32);
    u *= 6906969069U;
    u = u ^ rotl64(u, 17) ^ rotl64(u, 53);
    return u;
}


static void Mwc4159u64State_init(Mwc4159u64State *obj, const uint64_t seed[2])
{
    obj->pos = MWC4159_LAG;
    obj->c = 1234567890U; // Must be less than the multiplier
    uint64_t st0 = seed[0], st1 = seed[1];
    for (int i = 0; i < MWC4159_LAG; i++) {
        obj->x[i] = tf0sc_next(&st0);
        obj->x[i] += tf0sc_next(&st1);
    }
    // Warmup
    for (int i = 0; i < MWC4159_LAG; i++) {
        Mwc4159u64State_fill_buffer(obj);
    }
}

static inline uint64_t get_bits_raw(Mwc4159u64State *obj)
{
    if (obj->pos == MWC4159_LAG) {
        Mwc4159u64State_fill_buffer(obj);
        obj->pos = 0;
    }
    return obj->x[obj->pos++];
}

/**
 * @brief An internal self-test
 * @details Python 3.x script for generation of an internal self-test value:
 *
 *    import sympy, math
 *    a, b = 0xf6a64c36f2d94699, 2**64
 *    m = a*b**64 - 1
 *    o = sympy.n_order(b, m)
 *    print(sympy.isprime(m), 2*o - m == -1)
 *    print("log2(period) = ", math.log2(o))
 *    a_lcg = pow(b, -1, m)
 *    x = b**64 # c = 1
 *    for i in range(64):
 *        x = x + i * b**i
 *
 *    n = 1_000_000
 *    for ii in range(n + 63):
 *        x = (a_lcg * x) % m
 *    if ii >= n - 5 + 64:
 *        print(hex(x % b))
 */
static int run_self_test(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x6829d4afd0ddd401;
    Mwc4159u64State *obj = intf->malloc(sizeof(Mwc4159u64State));    
    for (unsigned int i = 0; i < MWC4159_LAG; i++) {
        obj->x[i] = i;
    }
    obj->c = 1;
    obj->pos = MWC4159_LAG;
    for (long i = 0; i < 1000000; i++) {
        u = get_bits_raw(obj);
    }
    intf->free(obj);
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


static void *create(const CallerAPI *intf)
{
    uint64_t seed[2];
    Mwc4159u64State *obj = intf->malloc(sizeof(Mwc4159u64State));
    seed[0] = intf->get_seed64();
    seed[1] = intf->get_seed64();
    Mwc4159u64State_init(obj, seed);
    return obj;
}

MAKE_UINT64_PRNG("Mwc4159_u64", run_self_test)
