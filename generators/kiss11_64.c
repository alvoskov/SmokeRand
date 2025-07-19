/**
 * @file kiss11_64.c
 * @brief A 64-bit modification of KISS from 2011 with huge period
 * and huge state.
 * @details It uses a combination of SuperDuper64 and MWC (multiply-with-carry).
 * An exact period is unknown but Marsaglia estimates it larger than 10^{40000000}
 * The MWC generator in this 64-bit version uses the next modulus:
 *
 * \f[
 *   m = (2^{28} - 1) b^{2^{22}} - 1 = (2 ^ {28} - 1) b^{2097152} - 1;~ b=2^{64}
 * \f]
 *
 * It is unknown if it is prime.
 *
 * References:
 *
 * 1. https://mathforum.org/kb/message.jspa?messageID=7359611
 * 
 * @copyright The KISS2011 algorithm was developed by George Marsaglia.
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
 * @brief KISS-2011 generator state, 64-bit version.
 */
typedef struct {
    uint64_t q[2097152]; ///< MWC state (values)
    uint64_t carry; ///< MWC state (carry)
    uint64_t cng; ///< 64-bit LCG state.
    uint64_t xs; ///< xorshift64 state
    int j; ///< Offset inside MWC circle buffer `q`.
} Kiss2011u64State;


/**
 * @brief MWC generator iteration.
 * @details It uses the next modulus:
 *
 * \f[
 *   m = (2^{28} - 1) b^{2^{22}} - 1 = (2 ^ {28} - 1) b^{2097152} - 1;~ b=2^{64}
 * \f]
 *
 * Usage of the \f$ a = 2^{28} - 1 \f$ multiplier allows to use bithacks instead
 * of 128-bit multiplication.
 */
static inline uint64_t Kiss2011u64State_mwc_iter(Kiss2011u64State *obj)
{
    obj->j = (obj->j + 1) & 2097151;
    uint64_t x = obj->q[obj->j];
    uint64_t t = (x << 28) + obj->carry;
    obj->carry = (x >> 36) - (t < x);
    return obj->q[obj->j] = t - x;
}

/**
 * @brief SuperDuper64 (LCG64 + xorshift64) subgenerator iteration.
 */
static inline uint64_t Kiss2011u64State_supdup_iter(Kiss2011u64State *obj)
{
    obj->cng = 6906969069ull * obj->cng + 13579ull;
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 43;
    return obj->cng + obj->xs;
}

/**
 * @brief Initialize the PRNG state: seeds are used to initialize SuperDuper
 * subgenerator that is used to initialize the MWC state.
 */
static void Kiss2011u64State_init(Kiss2011u64State *obj, uint64_t cng, uint64_t xs)
{
    // Initialize the LCG + XORSHIFT part.
    obj->cng = cng;
    obj->xs = xs;
    if (obj->xs == 0) {
        obj->xs = 362436069362436069ULL;
    }
    // Initialize the MWC state with LCG + XORSHIFT combination.
    // It is an essentially SuperDuper64 modification.
    obj->carry = 0;
    obj->j = 2097151;
    for (int i = 0; i < 2097152; i++) {
        obj->q[i] = Kiss2011u64State_supdup_iter(obj);
    }
}

static inline uint64_t get_bits_raw(void *state)
{
    uint64_t sd  = Kiss2011u64State_supdup_iter(state);
    uint64_t mwc = Kiss2011u64State_mwc_iter(state);
    return sd + mwc;
}


/**
 * @brief An internal self-test based on Marsaglia original code.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint64_t mwc_ref = 13596816608992115578ULL;
    const uint64_t kiss_ref = 5033346742750153761ULL;
    uint64_t x;
    int is_ok = 1;
    Kiss2011u64State *obj = intf->malloc(sizeof(Kiss2011u64State));
    Kiss2011u64State_init(obj, 123456789987654321ULL, 362436069362436069ULL);
    // Test MWC part
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = Kiss2011u64State_mwc_iter(obj);
    }
    intf->printf("Output: %llu; reference: x=%llu\n",
        (unsigned long long) x, (unsigned long long) mwc_ref);
    if (mwc_ref != x) {
        is_ok = 0;
    }
    // Test KISS part
    for (unsigned long i = 0; i < 1000000000;i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("Output: %llu; reference: x=%llu\n",
        (unsigned long long) x, (unsigned long long) kiss_ref);
    if (kiss_ref != x) {
        is_ok = 0;
    }
    intf->free(obj);
    return is_ok;
}

static void *create(const CallerAPI *intf)
{
    Kiss2011u64State *obj = intf->malloc(sizeof(Kiss2011u64State));
    Kiss2011u64State_init(obj, intf->get_seed64(), intf->get_seed64());
    return obj;
}

MAKE_UINT64_PRNG("KISS2011_u64", run_self_test)
