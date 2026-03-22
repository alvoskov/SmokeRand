/**
 * @file ranecu.c
 * @brief ranecu (or CombLec88) is a combination of two 31-bit LCGs with prime
 * multipliers.
 * @details It is a 31-bit generator that should be tested with the
 * `--filter=uint31` key. It systematically fails the `bspace16_4d_high` test
 * in the `default` and `full` SmokeRand batteries
 *
 * References:
 * 
 * 1. P. L'Ecuyer Efficient and portable combined random number generators //
 *    Communications of the ACM. 1998. V. 31. N 6. P.742-751
 *    https://doi.org/10.1145/62959.62969
 *
 * Python script for an internal self-test generation:
 *
 *    import sympy
 *    a0, a1 = 40014, 40692
 *    m0, m1, s = 2147483563, 2147483399, [1, 2]
 *    print('m0 is prime: ', sympy.isprime(m0))
 *    print('  Period: ', sympy.n_order(a0, m0), '; m0 = ', m0)
 *    print('m1 is prime: ', sympy.isprime(m1))
 *    print('  Period: ', sympy.n_order(a1, m1), '; m1 = ', m1)
 *    for i in range(10000000):
 *        s[0], s[1] = (a0 * s[0]) % m0, (a1 * s[1]) % m1
 *    for i in range(16):
 *        s[0], s[1] = (a0 * s[0]) % m0, (a1 * s[1]) % m1
 *        z = (s[0] - s[1]) % 2147483562
 *        print(z)
 *
 * Interesting Crush results:
 *
 *           Test                          p-value
 *     ----------------------------------------------
 *     13  BirthdaySpacings, t = 4        1.6e-90
 *     ----------------------------------------------
 *     All other tests were passed
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    int32_t s[2];
} RanecuState;

#define MOD0 2147483563
#define MOD1 2147483399

static inline void lcg31(int32_t *s, int32_t a, int32_t m, int32_t r, int32_t q)
{
    const int32_t k0 = *s / q;
    *s = a * (*s - k0 * q) - k0 * r;
    if (*s < 0) *s += m;
}


static inline uint64_t get_bits_raw(RanecuState *obj)
{
    lcg31(&obj->s[0], 40014, MOD0, 12211, 53668);
    lcg31(&obj->s[1], 40692, MOD1, 3791,  52774);
    int32_t z = obj->s[0] - obj->s[1];
    if (z < 1) z += 2147483562; // 2^31 - 86
    return (uint32_t)z << 1;
}

static void *create(const CallerAPI *intf)
{
    RanecuState *obj = intf->malloc(sizeof(RanecuState));
    obj->s[0] = (int32_t) (intf->get_seed64() % MOD0);
    obj->s[1] = (int32_t) (intf->get_seed64() % MOD1);
    if (obj->s[0] == 0) obj->s[0] = 1234567;
    if (obj->s[1] == 0) obj->s[1] = 7654321;
    return obj;
}

/**
 * @brief An internal self-test for the ranecu PRNG; the test vectors
 * were obtained from the Python 3.x test script.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[16] = {
         429125502, 1476861953,  171830325,   14926234,
        1437459321,  797276583, 1874442679,  270071228,
        1198654916,  174821678, 2039863595,  611866598,
        1095931304, 1807700546, 2095954574, 1798312916
    };
    RanecuState obj = {.s = {1, 2}};
    int is_ok = 1;
    for (long i = 0; i < 10000000; i++) {
        (void) get_bits_raw(&obj);
    }
    for (int i = 0; i < 16; i++) {
        uint32_t u = (uint32_t) get_bits_raw(&obj) >> 1;
        intf->printf("%10lu %10lu\n", (unsigned long) u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT32_PRNG("RANECU", run_self_test)
