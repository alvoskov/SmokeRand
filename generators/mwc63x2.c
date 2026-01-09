/**
 * @file mwc63x2.c
 * @brief Mwc63x2 combined PRNG: consists of two MWC (multiply-with-carry)
 * generators.
 * @details It is a combination of two MWC (Multiply-With-Carry) generators
 * designed to the signed 64-bit integers typical for Java or Oberon dialects.
 * This generator doesn't use integer overflows. The algorithm is fairly
 * robust and passes the next tests:
 *
 * With intentionally bad multipliers (4005 and 3939):
 *
 * - Passes `express`, `brief`, `default`, `full`.
 * - Passes SmallCrush, Crush, BigCrush from TestU01.
 * - PractRand: >= 2 TiB.
 *
 * With good multipliers:
 *
 * - Passes `express`, `brief`, `default`, `full`.
 * - Passes SmallCrush, Crush, BigCrush from TestU01.
 * - PractRand: >= 16 TiB.
 *
 * It also passes SmokeRand and TestU01 with reverse order of bits.
 *
 * MWC is rather famous algorithm developed by G. Marsaglia, it is an
 * essentially LCG with prime modulo in a specific form that allows very
 * fast high precision arithmetics.
 *
 * \f$ m = ab - 1 \f$ (where \f$b = 2^{31}\f$ for this implementation,
 * also \f$ a < b \f$)
 *
 * The interal state of the corresponding LCG is \f$ u = bc + x \f$ where c is
 * the carry, the x is the lower part of the multiplication result.
 * We can use the \f$ a \pm m \mod m = a \mod m \f$ formula to prove
 * the connection:
 *
 * \f[
 * a(bc + x) \mod m = (abc + ax) \mod m = \left(ab(c - 1) + ax + 1\right) \mod m =
 * ... = \left( ax + c \right) \mod m = ax + c.
 * \f]
 * 
 * mod m was excluded due to restrictions on initialization.
 *
 * References:
 *
 * 1. George Marsaglia. Random Number Generators // Journal of Modern Applied
 *    Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *    https://doi.org/10.22237/jmasm/1051747320
 * 2. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 3. https://github.com/lpareja99/spectral-test-knuth
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define MASK32 0xFFFFFFFF

/**
 * @brief MWC63x2 state.
 */
typedef struct {
    int64_t mwc1;
    int64_t mwc2;
} MWC63x2State;


static inline uint64_t get_bits_raw(MWC63x2State *obj)
{
    static const int64_t A0 = 1073100393, A1 = 1073735529;
    // static const int64_t A0 = 4005, A1 = 3939; // Bad multipliers
    // MWC 1 iteration
    const int64_t c1 = obj->mwc1 >> 32, x1 = obj->mwc1 & MASK32;
    obj->mwc1 = A0 * x1 + c1;
    // MWC 2 iteration
    const int64_t c2 = obj->mwc2 >> 32, x2 = obj->mwc2 & MASK32;
    obj->mwc2 = A1 * x2 + c2;
    // Output function
    int64_t out = (x1 + x2 + c1 + c2) & MASK32;
    return (uint32_t) out;
}


static void *create(const CallerAPI *intf)
{
    MWC63x2State *obj = intf->malloc(sizeof(MWC63x2State));
    // Seeding: prevent (0,0) and (?,0xFFFF)
    do {
        obj->mwc1 = (int64_t) (intf->get_seed64() >> 24);
    } while (obj->mwc1 == 0);
    do {
        obj->mwc2 = (int64_t) (intf->get_seed64() >> 24);
    } while (obj->mwc2 == 0);
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    MWC63x2State obj = {.mwc1 = 0x123DEADBEEF, .mwc2 = 0x456CAFEBABE};
    uint64_t u, u_ref = 0x9248038F; // For bad multipliers: u_ref = 0xD327A97A;
    for (long i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
        //intf->printf("%llX %llX %llX\n", u, obj.mwc1, obj.mwc2);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("MWC63x2", run_self_test)
