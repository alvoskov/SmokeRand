/**
 * @file skiss64.c
 * @brief SuperKISS64 pseudorandom number generator by G. Marsaglia.
 * @details Resembles KISS64/KISS99 but uses CMWC generator with a huge state
 * instead of two small MWC generators. The period is larger than 2^(1.3e6).
 *
 * The CMWC generator is based on the next prime found by G. Marsaglia:
 *
 * \f[
 * p = (2^{9} + 2^{7}) * b ^ 41265 + 1 =
 *     (2^{41} + 2^{39}) * B ^ {20632} + 1 =
 *     5 * 2^{1320487} + 1.
 * \f]
 *
 * where \f$ b = 2^{32} \f$ and \f$ b = 2^{64} \f$. A special form of
 * multipliers allows to use bithacks instead of 64-bit or 128-bit wide
 * multiplication.
 *
 * Citation from post by G. Marsaglia about period of the generator:
 *
 * That prime came from the many who have dedicated their
 * efforts and computer time to prime searches. After some
 * three weeks of dedicated computer time using pfgw with
 * scrypt, I found the orders of b and B:
 * 5*2^1320481 for b=2^32, 5*2^1320480 for B=2^64.
 *
 * References:
 *
 * 1. George Marsaglia. SuperKISS for 32- and 64-bit RNGs in both C and Fortran.
 *    https://www.thecodingforums.com/threads/superkiss-for-32-and-64-bit-rngs-in-both-c-and-fortran.706893/
 * 2. http://forums.silverfrost.com/viewtopic.php?t=1480
 * 3. George Marsaglia. Random Number Generators // Journal of Modern Applied
 *    Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *    https://doi.org/10.22237/jmasm/1051747320
 *
 * @copyright The SuperKISS algorithm was developed by George Marsaglia.
 *
 * Reentrant version for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SuperKISS64 generator state.
 */
typedef struct {
    uint64_t q[20632]; ///< CMWC state (values)
    uint64_t carry; ///< CMWC state (carry)
    uint64_t cng; ///< 32-bit LCG state.
    uint64_t xs; ///< xorshift32 state
    int i; ///< Offset inside CMWC circle buffer `q`.
} SuperKiss64State;

/**
 * @brief SuperDuper64 (LCG64 + xorshift64) subgenerator iteration.
 */
static inline uint64_t SuperKiss64State_supdup_iter(SuperKiss64State *obj)
{
    obj->cng = 6906969069ull * obj->cng + 123ull;
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 43;
    return obj->cng + obj->xs;
}

/**
 * @brief CMWC subgenerator.
 * @details It is based on the next recurrent formula:
 * \f[
 * x_{n} = (b-1) - \left( ax_{n - r} + c_{n - 1} \mod b \right)
 * \f]
 */
static inline uint64_t SuperKiss64State_cmwc_iter(SuperKiss64State *obj)
{
    if (obj->i >= 20632) {
        for (int i = 0; i < 20632; i++) {
            uint64_t q = obj->q[i];
            uint64_t h = (obj->carry & 1);
            uint64_t z = ((q << 41) >> 1) + ((q << 39) >> 1) + (obj->carry >> 1);
            obj->carry = (q >> 23) + (q >> 25) + (z >> 63);
            obj->q[i] = ~((z << 1) + h);
        }
        obj->i = 0;
    }
    return obj->q[obj->i++];
}

/**
 * @brief Initialize the PRNG state: seeds are used to initialize SuperDuper
 * subgenerator that is used to initialize the CMWC state.
 */
static void SuperKiss64State_init(SuperKiss64State *obj, uint64_t cng, uint64_t xs)
{
    obj->i = 20632;
    obj->carry = 36243678541ULL;
    obj->cng = cng;
    obj->xs = xs;
    if (obj->xs == 0) {
        obj->xs = 521288629546311ULL;
    }
    // Initialize the MWC state with LCG + XORSHIFT combination.
    // It is an essentially SuperDuper64 modification.
    for (int i = 0; i < 20632; i++) {
        obj->q[i] = SuperKiss64State_supdup_iter(obj);
    }
}


static inline uint64_t get_bits_raw(void *state)
{
    uint64_t sd   = SuperKiss64State_supdup_iter(state);
    uint64_t cmwc = SuperKiss64State_cmwc_iter(state);
    return sd + cmwc;
}

/**
 * @brief An internal self-test based on Marsaglia original code.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint64_t kiss_ref = 4013566000157423768ULL;
    uint64_t x;
    SuperKiss64State *obj = intf->malloc(sizeof(SuperKiss64State));
    SuperKiss64State_init(obj, 12367890123456ULL, 521288629546311ULL);
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("Output: %llu; reference: x=%llu\n",
        (unsigned long long) x, (unsigned long long) kiss_ref);
    intf->free(obj);
    return kiss_ref == x;
}


static void *create(const CallerAPI *intf)
{
    SuperKiss64State *obj = intf->malloc(sizeof(SuperKiss64State));
    SuperKiss64State_init(obj, intf->get_seed64(), intf->get_seed64());
    return obj;
}


MAKE_UINT64_PRNG("SuperKiss64", run_self_test)
