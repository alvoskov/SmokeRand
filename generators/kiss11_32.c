/**
 * @file kiss11_32.c
 * @brief A 32-bit modification of KISS from 2011 with huge period
 * and huge state.
 * @details It uses a combination of SuperDuper32 and MWC (multiply-with-carry).
 * An exact period is unknown but Marsaglia estimates it larger than 10^{40000000}
 * The MWC generator in this 64-bit version uses the next modulus:
 *
 * \f[
 *   m = (2^{28} - 1) b^{2^{21}} - 1 = (2 ^ {28} - 1) b^{4194304} - 1;~ b=2^{32}
 * \f]
 *
 * It is unknown if it is prime.
 *
 * References:
 *
 * 1. G. Marsaglia. RNGs with periods exceeding 10^(40million).
 *    https://mathforum.org/kb/message.jspa?messageID=7359611
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
 * @brief KISS-2011 generator state, 32-bit version.
 */
typedef struct {
    uint32_t q[4194304]; ///< MWC state (values)
    uint32_t carry; ///< MWC state (carry)
    uint32_t cng; ///< 32-bit LCG state.
    uint32_t xs; ///< xorshift32 state
    int j; ///< Offset inside MWC circle buffer `q`.
} Kiss2011u32State;


/**
 * @brief MWC generator iteration.
 * @details It uses the next modulus:
 *
 * \f[
 *   m = (2^{28} - 1) b^{2^{21}} - 1 = (2 ^ {28} - 1) b^{4194304} - 1;~ b=2^{32}
 * \f]
 *
 * Usage of the \f$ a = 2^{28} - 1 \f$ multiplier allows to use bithacks instead
 * of 64-bit multiplication.
 */
static inline uint32_t Kiss2011u32State_mwc_iter(Kiss2011u32State *obj)
{
    obj->j = (obj->j + 1) & 4194303;
    uint32_t x = obj->q[obj->j];
    uint32_t t = (x << 28) + obj->carry;
    obj->carry = (x >> 4) - (t < x);
    return obj->q[obj->j] = t - x;
}

/**
 * @brief SuperDuper32 (LCG32 + xorshift32) subgenerator iteration.
 */
static inline uint32_t Kiss2011u32State_supdup_iter(Kiss2011u32State *obj)
{
    obj->cng = 69069u * obj->cng + 13579u;
    obj->xs ^= obj->xs << 13;
    obj->xs ^= obj->xs >> 17;
    obj->xs ^= obj->xs << 5;
    return obj->cng + obj->xs;
}

/**
 * @brief Initialize the PRNG state: seeds are used to initialize SuperDuper
 * subgenerator that is used to initialize the MWC state.
 */
static void Kiss2011u32State_init(Kiss2011u32State *obj, uint32_t cng, uint32_t xs)
{
    obj->j = 4194303;
    obj->carry = 0;
    obj->cng = cng;
    obj->xs = xs;
    if (obj->xs == 0) {
        obj->xs = 362436069;
    }
    // Initialize the MWC state with LCG + XORSHIFT combination.
    // It is an essentially SuperDuper64 modification.
    for (int i = 0; i < 4194304; i++) {
        obj->q[i] = Kiss2011u32State_supdup_iter(obj);
    }
}

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t sd  = Kiss2011u32State_supdup_iter(state);
    uint32_t mwc = Kiss2011u32State_mwc_iter(state);
    return sd + mwc;
}

/**
 * @brief An internal self-test based on Marsaglia original code.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t mwc_ref = 2769813733, kiss_ref = 3545999299;
    uint32_t x;
    int is_ok = 1;
    Kiss2011u32State *obj = intf->malloc(sizeof(Kiss2011u32State));
    Kiss2011u32State_init(obj, 123456789, 362436069);
    // Test MWC part
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = Kiss2011u32State_mwc_iter(obj);
    }
    intf->printf("Output: %lu; reference: x=%lu\n",
        (unsigned long) x, (unsigned long) mwc_ref);
    if (mwc_ref != x) {
        is_ok = 0;
    }
    // Test KISS part
    for (unsigned long i = 0; i < 1000000000;i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("Output: %lu; reference: x=%lu\n",
        (unsigned long) x, (unsigned long) kiss_ref);
    if (kiss_ref != x) {
        is_ok = 0;
    }
    intf->free(obj);
    return is_ok;
}

static void *create(const CallerAPI *intf)
{
    Kiss2011u32State *obj = intf->malloc(sizeof(Kiss2011u32State));
    uint64_t s = intf->get_seed64();
    Kiss2011u32State_init(obj, s >> 32, (uint32_t) s);
    return obj;
}

MAKE_UINT32_PRNG("KISS2011_u32", run_self_test)
