/**
 * @file lcg127prime.c
 * @brief A classical 127-bit MCG with prime modulus that returns only
 * lower bits of its state.
 * @details The next recurrent formula is used:
 *
 * \f[
 * x_{n+1} = ax_{n} \mod (2^{127} - 1)
 * \f]
 *
 * Only lower 64 bits are returned.
 *
 * Some multipliers:
 *
 * a                    | PractRand 0.96 | SmokeRand | TestU01
 * ---------------------|----------------|-----------|-------------
 * 13433445539930070091 | >= 16 TiB      | full      | +HI/+LO/+IL
 * 13732206192813620784 | >= 16 TiB      | full      | +HI/+LO/+IL
 * 13765565284850700217 | >= 16 TiB      | full      | +HI/+LO/+IL
 *
 *
 * Experimental multiplier with worse spectral properties:
 *
 * The multiplier is 17937094319065857486 (`0xf8ed5c3f9698fdce`), it provides
 * the maximal period \f$ m - 2 \f$. Spectral test results:
 *
 *    t = 2; mu = 1.8349; fd = .7112; log2(v) = 63; v2 = 99374208123136570596894628735347314690
 *    t = 3; mu = 1.6861; fd = .6578; log2(v) = 41; v2 = 16739627555523343660972826
 *    t = 4; mu = 1.2675; fd = .5986; log2(v) = 31; v2 = 6610860253200439842
 *    t = 5; mu = 1.1204; fd = .5960; log2(v) = 24; v2 = 1055739544879002
 *    t = 6; mu = 1.7299; fd = .6457; log2(v) = 20; v2 = 3847514297892
 *    t = 7; mu = 1.6635; fd = .6400; log2(v) = 17; v2 = 62167354038
 *    t = 8; mu = .5754;  fd = .5539; log2(v) = 15; v2 = 2216161376
 *
 *    Merit = .5539; Merit_H = .6570
 *
 * The period can be verified by the next Python 3.x script:
 *
 *    import sympy
 *    sympy.n_order(17937094319065857486, 2**127 - 1) - (2**127 - 1)
 *
 * MCG uses a bithack for relatively fast (1.0 cpb) implementation based on
 * a trick from [1]. Of course it is fairly slow (comparable to hardware
 * accelerated AES or ChaCha) but is usually much faster than a classical
 * minstd implementation based on Schrage's algorithm (on modern x86-64 CPUs).
 * Of course quality of our 127-bit MCG is much higher than minstd.
 *
 * \f[
 * u \mod (2^{127} - 1) = H 2^{127} + L \mod m = H\cdot m + H + L \mod m =
 * H + L \mod m
 * \f]
 *
 * The \f$ H + L \f$ sum is always less than \f$ 2m \f$ that allows
 * to exclude division.
 *
 * References:
 *
 * 1. https://programmingpraxis.com/2014/01/14/minimum-standard-random-number-generator/
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));    
    Lcg128State_seed(obj, intf);
    obj->x_high >>= 1;
    obj->x_low |= 1;
    return obj;
}


static inline uint64_t get_bits_generic_raw(Lcg128State *obj, uint64_t a)
{
    static const uint64_t mask = 0x7fffffffffffffffU;
    uint64_t m_buf[3], t_hi;
    // a*x
    m_buf[0] = unsigned_mul128(a, obj->x_low, &t_hi);
    m_buf[1] = unsigned_muladd128(a, obj->x_high, t_hi, &m_buf[2]);
    // m = h*(2**127 - 1) + l where l <= 2**127 - 1
    const uint64_t h = (m_buf[2] << 1) | (m_buf[1] >> 63);
    m_buf[1] &= mask;
    // l += h
    unsigned_add128(&m_buf[1], &m_buf[0], h);
    // Note: 1) 2^127 - 1 cannot appear because it will be reduced to 0
    // (and it is prevented by a correct initializaton)
    // 2) This step will generate garbage in the highest bit of `x_high`
    // (because we subtract 2**128 - 1 instead of 2**127 - 1 but it will
    // be excluded anyway during the `m_buf[1] &= mask` step.
    if (m_buf[1] >> 63 != 0) {
        unsigned_add128(&m_buf[1], &m_buf[0], 1U);
    }
    // Update the state
    obj->x_low = m_buf[0];
    obj->x_high = m_buf[1];
    // Return the lower 64 bits
    return obj->x_low;
}


static inline uint64_t get_bits_mul1_raw(Lcg128State *obj)
{
    return get_bits_generic_raw(obj, 13433445539930070091U);
}

MAKE_GET_BITS_WRAPPERS(mul1)


static inline uint64_t get_bits_mul2_raw(Lcg128State *obj)
{
    return get_bits_generic_raw(obj, 13732206192813620784U);
}

MAKE_GET_BITS_WRAPPERS(mul2)


static inline uint64_t get_bits_mul3_raw(Lcg128State *obj)
{
    return get_bits_generic_raw(obj, 13765565284850700217U);
}

MAKE_GET_BITS_WRAPPERS(mul3)



/**
 * @brief An internal self-test based on Python 3.x generated values.
 * @details The next script was used:
 *
 *    a, x = 13433445539930070091, 1
 *    for i in range(1000000):
 *        x = (a*x) % (2**127 - 1)
 *    print(hex(x % 2**64))
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg128State obj = {.x_low = 1, .x_high = 0};
    uint64_t u, u_ref = 0xe490c2a6c3e38bcd ;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_mul1_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "Lcg127prime:mul1", 64, default_create, get_bits_mul1, get_sum_mul1},
    {"mul1",      "Lcg127prime:mul1", 64, default_create, get_bits_mul1, get_sum_mul1},
    {"mul2",      "Lcg127prime:mul2", 64, default_create, get_bits_mul2, get_sum_mul2},
    {"mul3",      "Lcg127prime:mul3", 64, default_create, get_bits_mul3, get_sum_mul3},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The x = ax mod 2**127 - 1 LCG that returns the lower 64 bits.\n"
"The next param values are supported:\n"
"  mul1 - a = 13433445539930070091 (default version)\n"
"  mul2 - a = 13732206192813620784\n"
"  mul3 - a = 13765565284850700217\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}

