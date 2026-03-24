/**
 * @file mwc2110_u64.c
 * @brief A 64-bit MWC generator with large lag (32) and base 2^64 - 1.
 * Has period around 2^2110.
 * @details
 *    import sympy, random
 *    b = 2**64 - 1
 *    random.seed(12345)
 *    ncandidates = 0
 *    for i in range(1000000):
 *        a = random.randint(2**62, 2**64 - 2)
 *        m = a*b**32 - 1
 *        if sympy.isprime(m):
 *            print(f"\nPrime candidate {a}")
 *            f = sympy.factorint((m - 1)//2, limit=10_000)
 *            if all(map(lambda x: sympy.isprime(x), f)):
 *                print("=====>", a, hex(a))
 *                if sympy.n_order(b, m) == m - 1:
 *                    print("^^^^^^^^^^")
 *                    break
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC8222 PRNG state.
 */
typedef struct {
    uint64_t x[32]; ///< Generated values
    uint64_t c; ///< Carry
    unsigned int pos; ///< Current position in the buffer
} Mwc2048u64State;


static void Mwc2048u64State_init(Mwc2048u64State *obj, uint64_t seed)
{
    obj->pos = 32;
    obj->c = 1234567890U; // Must be less than the multiplier
    for (int i = 0; i < 32; i++) {
        uint64_t u = seed ^ (seed >> 32);
        u *= 6906969069U;
        u = u ^ rotl64(u, 17) ^ rotl64(u, 53);
        seed += seed * seed | 0x40000005;
        obj->x[i] = u;
        if (obj->x[i] + 1 == 0) {
            obj->x[i] = 0;
        }
    }
}


static void fill_buffer(Mwc2048u64State *obj)
{
    static const uint64_t MWC_A = 0x4ef2ab6150c177ae;
    for (int i = 0; i < 32; i++) {
        // t = a*x + c = H*2**64 + L = H*b + (H + L) = H*b + (Shi*2**64 + Slo) =
        // = H*b + Shi*b + Shi + Slo
        // 1) (obj->c, t_lo) <- (H, L)
        uint64_t t_lo = unsigned_muladd128(MWC_A, obj->x[i], obj->c, &obj->c);
        // 2) (t_hi, t_lo) <- (Shi, Slo) <- (H + L)
        uint64_t t_hi = 0;
        unsigned_add128(&t_hi, &t_lo, obj->c);
        t_lo += t_hi; // 3) 
        obj->c += t_hi;
        obj->x[i] = t_lo;
    }
}

static inline uint64_t get_bits_raw(Mwc2048u64State *obj)
{
    if (obj->pos == 32) {
        fill_buffer(obj);
        obj->pos = 0;
    }
    return obj->x[obj->pos++];
}

/**
 * @brief An internal self-test
 * @details Python 3.x script for generation of an internal self-test value:
 *
 *    import sympy, math
 *    a, b = 0x4ef2ab6150c177ae, 2**64 - 1
 *    m = a*b**32 - 1
 *    o = sympy.n_order(b, m)
 *    print(sympy.isprime(m), o == m - 1)
 *    print("log2(period) = ", math.log2(o))
 *    a_lcg = pow(b, -1, m)
 *    x = b**32 # c = 1
 *    for i in range(32):
 *        x = x + i * b**i
 *
 *    n = 1_000_000
 *    for ii in range(n + 31):
 *        x = (a_lcg * x) % m
 *    if ii >= n - 5 + 32:
 *        print(hex(x % b))
 */
static int run_self_test(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x390da689f519e678;
    Mwc2048u64State *obj = intf->malloc(sizeof(Mwc2048u64State));    
    for (unsigned int i = 0; i < 32; i++) {
        obj->x[i] = i;
    }
    obj->c = 1;
    obj->pos = 32;
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
    Mwc2048u64State *obj = intf->malloc(sizeof(Mwc2048u64State));
    Mwc2048u64State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT64_PRNG("Mwc2110_u64", run_self_test)
