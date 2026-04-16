/**
 * @file lcg128prime.c
 * @brief A classical 127-bit MCG with prime modulus that returns only
 * lower bits of its state.
 * @details The next recurrent formula is used:
 *
 * \f[
 * x_{n+1} = ax_{n} \mod (2^{128} - 159)
 * \f]
 *
 * It returns an interleaved stream of lower/upper halves of 128-bit state.
 * It may make it slightly faster than the the 127-bit LCG.
 *
 * Some multipliers:
 *
 * a                    | PractRand 0.96 | SmokeRand | TestU01
 * ---------------------|----------------|-----------|-------------
 * 17914599225194756760 | >= 16 TiB      | full      | +HI/+LO/+IL
 * 17739802050595713108 | >= 16 TiB      | full      | +HI
 * 18250064560775652398 | >= 16 TiB      | full      |
 *
 * Experimental multiplier with worse spectral properties:
 *
 * The multiplier is 17942575458882244789 (`0xf900d550e65150b5`), it provides
 * the maximal period \f$ m - 2 \f$. Spectral test results:
 *
 *    t = 2; mu = 2.9722; fd = .9051; log2(v) = 63; v2 = 321936014097683397162897932591717654522
 *    t = 3; mu = 3.8315; fd = .8648; log2(v) = 42; v2 = 45928766653590506105386514
 *    t = 4; mu = 1.5698; fd = .6315; log2(v) = 31; v2 = 10404325825432111263
 *    t = 5; mu = 2.1008; fd = .6759; log2(v) = 25; v2 = 1791315309226189
 *    t = 6; mu = 2.3299; fd = .6785; log2(v) = 21; v2 = 5353423428261
 *    t = 7; mu =  .7540; fd = .5716; log2(v) = 17; v2 = 60448319394
 *    t = 8; mu = 3.0476; fd = .6822; log2(v) = 15; v2 = 3998104308
 *    Merit = .5716; Merit_H = .7888
 *
 *
 * The period can be verified by the next Python 3.x script:
 *
 *    import sympy
 *    sympy.n_order(17942575458882244789, 2**128 - 159) - (2**128 - 159)
 *
 * MCG uses a bithack for relatively fast (1.0 cpb) implementation based on
 * a trick from [1]. Of course it is fairly slow (comparable to hardware
 * accelerated AES or ChaCha) but is usually much faster than a classical
 * minstd implementation based on Schrage's algorithm (on modern x86-64 CPUs).
 * Of course quality of our 127-bit MCG is much higher than minstd.
 *
 * \f[
 * u \mod (2^{128} - 159) = H 2^{128} + L \mod m = H\cdot m + 159\cdot H + L \mod m =
 * 159\mod H + L \mod m
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


typedef struct {
    uint64_t x_low;
    uint64_t x_high;
    int get_low;
} Lcg128PrimeState;


static void *create(const CallerAPI *intf)
{
    Lcg128PrimeState *obj = intf->malloc(sizeof(Lcg128PrimeState));
    obj->x_low = intf->get_seed64();
    obj->x_high = intf->get_seed64() >> 1;
    obj->x_low |= 1;
    obj->get_low = 1;
    return obj;
}

static inline uint64_t sub64b(uint64_t *a, uint64_t b)
{
    const uint64_t t = *a - b;
    const uint64_t c = (t > *a) ? 1 : 0; // borrow
    *a = t;
    return c;
}

static inline uint64_t get_bits_generic_raw(Lcg128PrimeState *obj, uint64_t a)
{
    static const uint64_t d = 159;
    if (obj->get_low) {
        uint64_t m_buf[3], mhi_buf[2], t_hi;
        // m = a*x
        m_buf[0] = unsigned_mul128(a, obj->x_low, &t_hi);
        m_buf[1] = unsigned_muladd128(a, obj->x_high, t_hi, &m_buf[2]);
        // mhi_buf = 159 * H
        mhi_buf[0] = unsigned_mul128(d, m_buf[2], &mhi_buf[1]);
        // 159*H + L; 159*H is inside m hi_buf
        const uint64_t mbuf1_in = m_buf[1];
        unsigned_add128(&m_buf[1], &m_buf[0], mhi_buf[0]);
        m_buf[2] = (mbuf1_in > m_buf[1]) ? 1 : 0; // carry
        unsigned_add128(&m_buf[2], &m_buf[1], mhi_buf[1]);
        // -= m if needed
        if (m_buf[2] != 0 || (~m_buf[1] == 0U &&
                               m_buf[0] >= 0xFFFFFFFFFFFFFF61U)) {
            unsigned_add128(&m_buf[1], &m_buf[0], 0x60U);
        }
        // Update the state
        obj->x_low = m_buf[0];
        obj->x_high = m_buf[1];
        // Return the lower 64 bits
        obj->get_low = 0;
        return obj->x_low;
    } else {
        // Return the higher 64 bits
        obj->get_low = 1;
        return obj->x_high;
    }
}


static inline uint64_t get_bits_mul1_raw(void *obj)
{
    return get_bits_generic_raw(obj, 17914599225194756760U);
}

MAKE_GET_BITS_WRAPPERS(mul1)


static inline uint64_t get_bits_mul2_raw(void *obj)
{
    return get_bits_generic_raw(obj, 17739802050595713108U);
}

MAKE_GET_BITS_WRAPPERS(mul2)


static inline uint64_t get_bits_mul3_raw(void *obj)
{
    return get_bits_generic_raw(obj, 18250064560775652398U);
}

MAKE_GET_BITS_WRAPPERS(mul3)



/**
 * @brief An internal self-test based on Python 3.x generated values.
 * @details The next script was used:
 *
 *    a, x = 17914599225194756760, 1
 *    for i in range(10_000_000):
 *        x = (a*x) % (2**128 - 159)
 *    print(hex(x % 2**64))
 */
static int run_self_test(const CallerAPI *intf)
{
    Lcg128PrimeState obj = {.x_low = 1, .x_high = 0, .get_low = 1};
    uint64_t u, u_ref = 0x66f9c443cb87deU;
    for (size_t i = 0; i < 20000000 - 1; i++) {
        u = get_bits_mul1_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


static const GeneratorParamVariant gen_list[] = {
    {"",          "Lcg128prime:mul1", 64, default_create, get_bits_mul1, get_sum_mul1},
    {"mul1",      "Lcg128prime:mul1", 64, default_create, get_bits_mul1, get_sum_mul1},
    {"mul2",      "Lcg128prime:mul2", 64, default_create, get_bits_mul2, get_sum_mul2},
    {"mul3",      "Lcg128prime:mul3", 64, default_create, get_bits_mul3, get_sum_mul3},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The x = ax mod 2**128 - 159 LCG that returns the lower 64 bits.\n"
"The next param values are supported:\n"
"  mul1 - a = 17914599225194756760 (default version)\n"
"  mul2 - a = 17739802050595713108\n"
"  mul3 - a = 18250064560775652398\n";



int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}

