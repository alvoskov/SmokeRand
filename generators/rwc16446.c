/**
 * @file rwc16446.c
 * @brief A 64-bit RWC generator with a large lag (256) and base \f$2^{64}-1\f$.
 * Has the period around \f$ 2^{16446} \f$ (\f$ \sim 2^{16446.565} \f$).
 * @details This PRNG was designed by A.L. Voskov. It uses the next recurrent
 * formula:
 *
 * \f[
 * u_i = a x_{i-256} + x_{i-255} + c_{i-1}
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

#define RWC16446_LAG 256
#define RWC16446_LAGMASK 0xFF

/**
 * @brief RWC16446 PRNG state.
 */
typedef struct {
    uint64_t x[RWC16446_LAG]; ///< Generated values
    uint64_t c; ///< Carry
    unsigned int i; ///< Current position in the buffer
} Rwc16446State;

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

static inline uint64_t Rwc16446State_next(Rwc16446State *obj)
{
    static const uint64_t a = 0xbd4e5d1a2d5969e5U; // 13640942688363178469 
    const unsigned int j = (obj->i + 1) & RWC16446_LAGMASK;
    const uint64_t x = obj->x[obj->i], y = obj->x[j]; // x_{n-256}, x_{n-255}
    uint64_t new_hi, new_lo;
    new_lo = unsigned_muladd128(a, x, y, &new_hi);
    unsigned_add128(&new_hi, &new_lo, obj->c);
    obj->x[obj->i] = new_lo;
    obj->c = new_hi;
    obj->i = j;
    return new_lo;
}

/**
 * @brief A modification with the raw output from the linear part.
 */
static inline uint64_t get_bits_linear_raw(Rwc16446State *obj)
{
    return Rwc16446State_next(obj);
}

MAKE_GET_BITS_WRAPPERS(linear)

/**
 * @brief A modification with the lightweight scrambler.
 */
static inline uint64_t get_bits_scrambled_raw(Rwc16446State *obj)
{    
    const uint64_t u = obj->x[obj->i];
    (void) Rwc16446State_next(obj);
    return u ^ rotl64(u, 17) ^ rotl64(u, 53);
}

MAKE_GET_BITS_WRAPPERS(scrambled)

/**
 * @brief Initializes the generator using the supplied 128-bit seed.
 * @details The seed is expanded by means of two simple nonlinear PRNGs
 * with subsequent "warmup" stage to decorrelate the PRNG copies.
 */
static void Rwc16446State_init(Rwc16446State *obj, const uint64_t seed[2])
{
    obj->i = RWC16446_LAG;
    obj->c = 1234567890U; // Must be less than the multiplier
    // Seed expansion
    uint64_t st0 = seed[0], st1 = seed[1];
    for (int i = 0; i < RWC16446_LAG; i++) {
        obj->x[i] = tf0sc_next(&st0);
        obj->x[i] += tf0sc_next(&st1);
    }
    // Warmup
    for (int i = 0; i < RWC16446_LAG * RWC16446_LAG; i++) {
        (void) get_bits_linear_raw(obj);
    }
}


/**
 * @brief An internal self-test
 * @details Python 3.x script for generation of an internal self-test value
 * can be found at `misc/rwc.py`.
 */
static int run_self_test(const CallerAPI *intf)
{
    Rwc16446State *obj = intf->malloc(sizeof(Rwc16446State));    
    for (unsigned int i = 0; i < RWC16446_LAG; i++) {
        obj->x[i] = i;
    }
    obj->c = 1;
    obj->i = 0;
    for (long i = 0; i < 100000; i++) {
        (void) get_bits_linear(obj);
    }
    const uint64_t u = get_bits_linear(obj), u_ref = 0x51B030FF96ACA476U;
    intf->free(obj);
    intf->printf("Output: 0x%llX; reference: 0x%llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}


static void *create(const CallerAPI *intf)
{
    uint64_t seed[2];
    Rwc16446State *obj = intf->malloc(sizeof(Rwc16446State));
    seed[0] = intf->get_seed64();
    seed[1] = intf->get_seed64();
    Rwc16446State_init(obj, seed);
    return obj;
}

static const GeneratorParamVariant gen_list[] = {
    {"",    "rwc16446rrx", 64, default_create, get_bits_scrambled, get_sum_scrambled},
    {"tr",  "rwc16446rrx", 64, default_create, get_bits_scrambled, get_sum_scrambled},
    {"lin", "rwc16446lin", 64, default_create, get_bits_linear,    get_sum_linear},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"RWC16446 is a modification of MWC (multiply with carry) generator with\n"
"two lags. Its period is about 2^16446. The default variant is equipped\n"
"with a lightweight scrambler. The next param values are supported:\n"
"  rrx - a modification with the scrambler (default)\n"
"  lin - just direct output without scrambler\n";

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
