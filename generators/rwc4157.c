/**
 * @file rwc4157.c
 * @brief A 64-bit RWC generator with a large lag (64) and base \f$2^{64}-1\f$.
 * Has the period around \f$ 2^{4157} \f$ (\f$ \sim 2^{4157.744} \f$).
 * @details This PRNG was designed by A.L. Voskov. It uses the next recurrent
 * formula:
 *
 * \f[
 * u_i = a x_{i-64} + x_{i-63} + c_{i-1}
 * \f]
 *
 * \f[
 * c_i = \left\lfloor \dfrac{u_i}{b} \right\rfloor;~
 * x_i = u_i \mod b
 * \f]
 *
 * where \f$ b = 2^{64} \f$. It is optimized for 64-bit CPUs and is usually
 * fairly fast, around 0.25-0.3 cpb.
 *
 * See the `mwc2110_u64` source code for Python 3.x script for multipliers
 * search!
 *
 * References:
 *
 * 1. https://www.stat.berkeley.edu/~spector/s243/mother.c
 * 2. M. Goresky, A. Klapper. Efficient multiply-with-carry random number
 *    generators with maximal period // ACM Trans. Model. Comput. Simul. 2003.
 *    V. 13. N 4. P. 310-321. https://doi.org/10.1145/945511.945514
 *
 * Note: both \f$ m \f$ and all \f$ m - 1 \f$ cofactors were proven to be prime
 * by means of Primo 4.3.3, see the misc/mwc2110_cert directory.
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

#define RWC4157_LAG 64

/**
 * @brief RWC4157 PRNG state.
 */
typedef struct {
    uint64_t x[RWC4157_LAG]; ///< Generated values
    uint64_t c; ///< Carry
    unsigned int i; ///< Current position in the buffer
} Rwc4157State;

/**
 * @brief A simple nonlinear generator based on the Klimov-Shamir "crazy"
 * TF0 function with some ad-hoc output scrambler.
 * @details See `tf0_sc2` PRNG for details. The generator quaility is fairly
 * high.
 */
static inline uint64_t tf0sc_next(uint64_t *state)
{
    *state += *state * *state | 0x40000005;
    uint64_t u = *state ^ (*state >> 32);
    u *= 6906969069U;
    u = u ^ rotl64(u, 17) ^ rotl64(u, 53);
    return u;
}


static inline uint64_t get_bits_raw(Rwc4157State *obj)
{
    static const uint64_t a = 0x6b2b8accbeb42f68U;
    const unsigned int j = (obj->i + 1) & 0x3F;
    const uint64_t x = obj->x[obj->i], y = obj->x[j]; // x_{n-64}, x_{n-63}
    uint64_t new_hi, new_lo;
    new_lo = unsigned_muladd128(a, x, y, &new_hi);
    unsigned_add128(&new_hi, &new_lo, obj->c);
    obj->x[obj->i] = new_lo;
    obj->c = new_hi;
    obj->i = j;
    return new_lo;
}

/**
 * @brief Initializes the generator using the supplied 128-bit seed.
 * @details The seed is expanded by means of two simple nonlinear PRNGs
 * with subsequent "warmup" stage to decorrelate the RWC4157 copies.
 */
static void Rwc4157State_init(Rwc4157State *obj, const uint64_t seed[2])
{
    obj->i = RWC4157_LAG;
    obj->c = 1234567890U; // Must be less than the multiplier
    // Seed expansion
    uint64_t st0 = seed[0], st1 = seed[1];
    for (int i = 0; i < RWC4157_LAG; i++) {
        obj->x[i] = tf0sc_next(&st0);
        obj->x[i] += tf0sc_next(&st1);
    }
    // Warmup
    for (int i = 0; i < RWC4157_LAG * RWC4157_LAG; i++) {
        get_bits_raw(obj);
    }
}


/**
 * @brief An internal self-test
 * @details Python 3.x script for generation of an internal self-test value
 * can be found at `misc/rwc.py`.
 */
static int run_self_test(const CallerAPI *intf)
{
    Rwc4157State *obj = intf->malloc(sizeof(Rwc4157State));    
    for (unsigned int i = 0; i < RWC4157_LAG; i++) {
        obj->x[i] = i;
    }
    obj->c = 1;
    obj->i = 0;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(obj);
    }
    const uint64_t u = get_bits_raw(obj), u_ref = 0x221D7F2EBE123A4BU;
    intf->free(obj);
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


static void *create(const CallerAPI *intf)
{
    uint64_t seed[2];
    Rwc4157State *obj = intf->malloc(sizeof(Rwc4157State));
    seed[0] = intf->get_seed64();
    seed[1] = intf->get_seed64();
    Rwc4157State_init(obj, seed);
    return obj;
}

MAKE_UINT64_PRNG("rwc4157", run_self_test)
