/**
 * @file skiss32.c
 * @brief SuperKISS32 pseudorandom number generator by G. Marsaglia.
 * @details Resembles KISS99 but uses CMWC generator with a huge state
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
 * @brief SuperKISS32 generator state.
 */
typedef struct {
    uint32_t q[41265]; ///< CMWC state (values)
    uint32_t carry; ///< CMWC state (carry)
    uint32_t cng; ///< 32-bit LCG state.
    uint32_t xs; ///< xorshift32 state
    int i; ///< Offset inside CMWC circle buffer `q`.
} SuperKiss32State;


/**
 * @brief SuperDuper32 (LCG32 + xorshift32) subgenerator iteration.
 */
static inline uint32_t SuperKiss32State_supdup_iter(SuperKiss32State *obj)
{
    obj->cng = 69069u * obj->cng + 123u;
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 5;
    return obj->cng + obj->xs;
}

/**
 * @brief CMWC subgenerator.
 * @details It is based on the next recurrent formula:
 * \f[
 * x_{n} = (b-1) - \left( ax_{n - r} + c_{n - 1} \mod b \right)
 * \f]
 */
static inline uint32_t SuperKiss32State_cmwc_iter(SuperKiss32State *obj)
{
    if (obj->i >= 41265) {
        for (int i = 0; i < 41265; i++) {
            uint32_t q = obj->q[i];
            uint32_t h = (obj->carry & 1);
            uint32_t z = ((q << 9) >> 1) + ((q << 7) >> 1) + (obj->carry >> 1);
            obj->carry = (q >> 23) + (q >> 25) + (z >> 31);
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
static void SuperKiss32State_init(SuperKiss32State *obj, uint32_t cng, uint32_t xs)
{
    obj->i = 41265;
    obj->carry = 362;
    obj->cng = cng;
    obj->xs = xs;
    if (obj->xs == 0) {
        obj->xs = 521288629;
    }
    // Initialize the MWC state with LCG + XORSHIFT combination.
    // It is an essentially SuperDuper64 modification.
    for (int i = 0; i < 41265; i++) {
        obj->q[i] = SuperKiss32State_supdup_iter(obj);
    }
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t sd   = SuperKiss32State_supdup_iter(state);
    uint32_t cmwc = SuperKiss32State_cmwc_iter(state);
    return sd + cmwc;
}

/**
 * @brief An internal self-test based on Marsaglia original code.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t kiss_ref = 1809478889;
    uint32_t x;
    SuperKiss32State *obj = intf->malloc(sizeof(SuperKiss32State));
    SuperKiss32State_init(obj, 1236789, 521288629);
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("Output: %lu; reference: x=%lu\n",
        (unsigned long) x, (unsigned long) kiss_ref);
    intf->free(obj);
    return kiss_ref == x;
}


static void *create(const CallerAPI *intf)
{
    SuperKiss32State *obj = intf->malloc(sizeof(SuperKiss32State));
    uint64_t s = intf->get_seed64();
    SuperKiss32State_init(obj, s >> 32, (uint32_t) s);
    return obj;
}


MAKE_UINT32_PRNG("SuperKiss32", run_self_test)
