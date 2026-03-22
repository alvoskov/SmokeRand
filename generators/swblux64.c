/**
 * @file swblux64.c
 * @brief A 64-bit version of the subtract-with-borrow (SWB) PRNG with luxury
 * levels support.
 * @details This generator is designed for 64-bit computers and is based on
 * the next recurrent formula:
 *
 * \f[
 * x_{n} = x_{n-s} - x_{n-r} - b_{n-1} \mod 2^{64}
 * \f]
 *
 * \f[
 * b_{n} = \begin{cases}
 *   0 & \textrm{ if } x_{n-s} - x_{n-r} - b_{n-1} \ge 0 \\
 *   1 & \textrm{ if } x_{n-s} - x_{n-r} - b_{n-1} <   0 \\
 * \end{cases}
 * \f]
 *
 * The \f$ r = 13 \f$ and \f$ s = 7 \f$ lags are selected by A.L. Voskov
 * to provide the \f$ m = 2^{64*13} - 2^{64*7} + 1 \f$ prime modulus. The
 * modulus m is Proth prime. The period of the SWB64 generator is about
 * \f$ 10^{246} \f$ or \f$ 2^{818} \f$ and can be estimated
 * by the next Python 3.x program:
 *
 *    import sympy, math
 *    b = 2**64
 *    m = b**13 - b**7 + 1
 *    print("Prime test from sympy: ", sympy.isprime(m))
 *    a = 23
 *    print("Proth test: ", pow(a, (m - 1) // 2, m) - (-1 % m) == 0)
 *    print("Finding generator period...")
 *    o = sympy.n_order(b, m) # or sympy.n_order(pow(b, -1, m), m)
 *    print("Period: ", o)
 *    print("log10(per): ", math.log10(o), "; log2(per)", math.log2(o))
 *
 * Luxury levels (decimation) improve PRNG quality, it is not recommended
 * to use the 0-2 levels in a general purpose PRNG.
 *
 * Note about --param=2 version: it does fail BigCrush (higher half):
 *
 *           Test                          p-value
 *     ----------------------------------------------
 *     10  CollisionOver, t = 14           3.2e-5
 *     11  CollisionOver, t = 21          3.8e-10
 *     12  CollisionOver, t = 21           1.9e-6
 *     ----------------------------------------------
 *
 * References:
 *
 * 1. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright The SWB algorithm was suggested by G.Marsaglia and A.Zaman.
 *
 * Tuning for 64-bit output and reentrant implementation for SmokeRand:
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 13
#define SWB_B 7

/**
 * @brief 64-bit Swb state.
 */
typedef struct {    
    uint64_t x[SWB_A];
    uint64_t c;
    int i;
    int j;
    int luxury;
    int pos;
} Swb64State;


static inline uint64_t get_bits_nolux(Swb64State *obj)
{
    // SWB part
    const uint64_t xj = obj->x[obj->j], xi = obj->x[obj->i];
    const uint64_t t = xj - xi - obj->c;
    obj->c = (xj < t) ? 1 : 0;
    obj->x[obj->i] = t;
    if (obj->i == 0) { obj->i = SWB_A; }
	if (obj->j == 0) { obj->j = SWB_A; }
    obj->i--;
    obj->j--;
    return t;
}


/**
 * @brief This wrapper implements "luxury levels".
 */
static inline uint64_t get_bits_raw(Swb64State *obj)
{
    if (++obj->pos == SWB_A) {
        obj->pos = 0;
        for (int i = 0; i < obj->luxury; i++) {
            get_bits_nolux(obj);
        }
    }
    return get_bits_nolux(obj);
}


static void *create_lux(const CallerAPI *intf, int luxury)
{
    Swb64State *obj = intf->malloc(sizeof(Swb64State));    
    for (size_t i = 0; i < SWB_A; i++) {
        obj->x[i] = intf->get_seed64();
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->i = SWB_A - 1; obj->j = SWB_B - 1;
    obj->pos = 0;
    obj->luxury = luxury;
    return obj;
}


static void *create(const CallerAPI *intf)
{
    return create_lux(intf, 0);
}

static void *create_lux0(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_lux(intf, 13 - 13);
}

static void *create_lux1(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_lux(intf, 26 - 13);
}

static void *create_lux2(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_lux(intf, 42 - 13);
}

/**
 * @brief 
 * @details mu = 1.5857; 3.9197; 4.3099; 2.4531; 0.2692;
 */
static void *create_lux3(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return create_lux(intf, 83 - 13);
}

EXPORT uint64_t get_bits(void *state) { return get_bits_raw(state); }
GET_SUM_FUNC

static const GeneratorParamVariant gen_list[] = {
    {"",   "swb64_lux0[13,13]", 64, create_lux0, get_bits, get_sum},
    {"0",  "swb64_lux0[13,13]", 64, create_lux0, get_bits, get_sum},
    {"1",  "swb64_lux1[13,26]", 64, create_lux1, get_bits, get_sum},
    {"2",  "swb64_lux2[13,42]", 64, create_lux2, get_bits, get_sum},
    {"3",  "swb64_lux3[13,83]", 64, create_lux3, get_bits, get_sum},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"swb64: 64-bit Subtract-with-Borrow generator with luxury levels support.\n"
"  x_n = x_(n-7) - x_(n-13) - b_(n-1) mod 2^64\n"
"The next param values are supported:\n"
"  0 - swb64_lux0[13,13] (default)\n"
"  1 - swb64_lux1[13,26]\n"
"  2 - swb64_lux2[13,42]\n"
"  3 - swb64_lux3[13,83]\n";

/**
 * @brief An internal self-test
 * @details The next code was used for the test vector generation:
 *
 *    class Swb64:
 *        def __init__(self):
 *            self.a, self.b = 13, 7
 *            self.x = [x + 1000 for x in range(0, self.a)]
 *            self.c = 1
 *
 *        def next(self):
 *            xj, xi = self.x[self.b - 1], self.x[self.a - 1]
 *            t = (xj - xi - self.c) % 2**64
 *            self.c = 1 if xj < t else 0
 *            self.x = [t] + self.x[:-1]
 *            return t
 *
 *    swb = Swb64()
 *
 *    for i in range(1_000_000):
 *        swb.next()
 *
 *    for i in range(16):
 *        print(hex(swb.next()))
 *
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = {
        0x69a9cdd6c63ed355, 0xc48119f8a063719a, 0x8d6a86605e7ec754,
        0x9c4a47702785aa51, 0xc2342980fb6d22a2, 0xdf0b316d28a4a53a,
        0xc92bc711d1225e9e, 0xb4d9cea46aedff76, 0x20e7bcda51f57749,
        0xe7547f8de529688e, 0xd6ddf16b6b07ea5d, 0x3b6dff7cadc502c4,
        0x89ccd703c6ccf913, 0x5f81f93b0ae38b49, 0xf058b4abca8a8ddc,
        0x937d3679f376aff4
    };

    int is_ok = 1;
    Swb64State *obj = create_lux(intf, 0);
    for (unsigned int i = 0; i < SWB_A; i++) {
        obj->x[i] = 1000U + i;
    }
    obj->c = 1;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits(obj);
    }
    for (int i = 0; i < 16; i++) {
        const uint64_t u = get_bits(obj);
        intf->printf("%llX %llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}

