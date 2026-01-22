/**
 * @file kiss03_64.c
 * @brief KISS03_64 is a 64-bit modification of the KISS03 generator suggested
 * by George Marsaglia. The modificaiton is made by A.L. Voskov and includes
 * an improved output function.
 * @details The next modifications were made by A.L. Voskov to adapt KISS03
 * to the 64-bit words:
 *
 * 1. A rotation for LCG was added to the output function: now both LCG+LFSR
 *    (i.e. "Super-Duper") and MWC parts pass SmokeRand `full` battery
 *    separately.
 * 2. MWC generator \f$ b = 2^{64} \f$ and \f$ a = 2^{64} - 10^{16} - 69888 \f$
 *    (\f$ 69888 = 273 \cdot 256 \f$): it was selected to be easy to memorise and
 *    have fairly good spectral characteristics (see `mwc128.c` for a slightly
 *    better multiplier).
 * 3. xorshift32 was replaced to xorshift64 with constants from [2].
 * 4. 32-bit LCG was replaced to the 64-bit LCG.
 *
 * This generator has a period around \f$ 2^{255} \f$:
 *
 * - xorshift64: \f$ 2^{64} - 1 \f$, LCG: \f$ 2^{64} \f$.
 * - MWC: \f$ a\cdot 2^{64} - 1 \approx 2^{127} \f$.
 *
 * Spectral test for \f$ t=2-8 \f$ (\f$ \mu \f$ values):
 *
 * - LCG: 1.7905 4.2978 2.9836 2.7304 2.3194 2.4991 0.5027
 * - MWC: 3.1398 0      4.7548 2.3007 1.0534 3.3183 0.7650
 *
 * References:
 *
 * 1. George Marsaglia. Random Number Generators // Journal of Modern Applied
 *    Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *    https://doi.org/10.22237/jmasm/1051747320
 * 2. Vigna S. An Experimental Exploration of Marsaglia's xorshift Generators,
 *    Scrambled // ACM Trans. Math. Softw. 2016. V. 42. N 4. Article ID 30.
 *    https://doi.org/10.1145/2845077
 * 3. https://www.thecodingforums.com/threads/64-bit-kiss-rngs.673657/
 *
 * @copyright The KISS03 algorithm is developed by George Marsaglia.
 *
 * 64-bit modification and its implementation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Kiss03x64 PRNG state.
 */
typedef struct {
    uint64_t x; ///< 64-bit LCG state
    uint64_t y; ///< xorshift64 state
    uint64_t z; ///< MWC state: lower part
    uint64_t c; ///< MWC state: higher part (carry)
} Kiss03x64State;


static inline uint64_t get_bits_raw(Kiss03x64State *obj)
{
    static const uint64_t MWC_A1 = 0xffdc790d903def00; // 2**64 - 10**16 - 69888
    // LCG part
    obj->x = 6906969069ULL * obj->x + 1234567ULL;
    // xorshift part
    obj->y ^= obj->y >> 12;
    obj->y ^= obj->y << 25;
    obj->y ^= obj->y >> 27;
    // MWC part
    obj->z = unsigned_muladd128(MWC_A1, obj->z, obj->c, &obj->c);
    // Combined output
    return rotl64(obj->x, 8) + obj->y + obj->z;
}


static void *create(const CallerAPI *intf)
{
    Kiss03x64State *obj = intf->malloc(sizeof(Kiss03x64State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->c = (intf->get_seed64() & 0xFFFFFFFFFFFF) + 1;
    if (obj->y == 0) {
        obj->y = 0x12345678;
    }
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t x_ref = 0xE78F04EE8307A14A;
    uint64_t x;
    Kiss03x64State *obj = intf->malloc(sizeof(Kiss03x64State));
    obj->x = 123456789; obj->y = 987654321;
    obj->z = 43219876;  obj->c = 6543217;
    for (long i = 0; i < 10000000; i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("Observed: 0x%llX; expected: 0x%llX\n",
        (unsigned long long) x, (unsigned long long) x_ref);
    intf->free(obj);
    return (x == x_ref) ? 1 : 0;
}


MAKE_UINT64_PRNG("KISS2003x64", run_self_test)
