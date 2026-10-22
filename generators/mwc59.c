/**
 * @file mwc59.c
 * @brief Multiply-with-carry generator that uses only 59 bits of state, its
 * period is only \f$2^{57.996} \f$.
 * @details This file contains three version of the generator:
 *
 * - `lin`, no scrambler, fails a lot of tests
 * - `sc1`, a weak scrambler, fails `bspace8_8d`, `bspace21_3d_c256` and
 *   `bspace8_8d_c256` tests from the SmokeRand `full` battery. May be
 *   suspicious in `bspace21_3d`.
 * - `sc2`, a strong scrambler.
 *
 * It is interesting that the `sc1` modification passes BigCrush:
 *
 *    ========= Summary results of BigCrush(mt) =========
 *
 *     Version:                      TestU01 1.2.3
 *     Generator:                    mwc59_sc1
 *     Number of statistics:         160
 *     Total CPU time (all cores):   03:55:26.578
 *     Elapsed time:                 00:50:43.329
 *     p-values checksum:            0x4BBBB3DC55AD1A89
 *
 *     All tests were passed
 *
 * MWC is a special form of multiplicative congruential generator with the
 * multiplier \f$a\f$ and prime modulus \f$m = ab - 1\f$.
 *
 * Spectral test results for the linear core:
 *
 *     a = 133850370; m = 574882961707499519
 *       t = 2; mu = .0979; fd = .1642; log2(v) = 26; v2 = 17915921549136901
 *       t = 3; mu = .0024; fd = .0740; log2(v) = 16; v2 = 4771303753
 *       t = 4; mu = 2.8084; fd = .7303; log2(v) = 14; v2 = 571993575
 *       t = 5; mu = 2.2726; fd = .6866; log2(v) = 11; v2 = 9076642
 *       t = 6; mu = 1.3809; fd = .6219; log2(v) = 9; v2 = 535576
 *       t = 7; mu = 3.2477; fd = .7042; log2(v) = 8; v2 = 106574
 *       t = 8; mu = 5.1608; fd = .7286; log2(v) = 7; v2 = 29240
 *       Merit = .0740; Merit_H = .3709
 *
 * The \f$f_2\f$ and \f$f_3\f$ values are in a perfect agreement with [1].
 *
 * References:
 * 
 * 1. Sebastiano Vigna. Scrambled Multiply-with-Carry Pseudorandom Number Generators
 *    https://arxiv.org/abs/2610.27313
 * 2. https://github.com/erlang/otp/blob/master/lib/stdlib/src/rand.erl
 *
 * @copyright The mwc59 generator was designed by Sebastiano Vigna, Raimo Niskanen.
 *
 * Reentrant C99 version for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define MWC59_A      0x7FA6502U
#define MWC59_MASK32 0xFFFFFFFFU

/**
 * @brief MWC59 state.
 */
typedef struct {
    uint64_t data;
} MWC59State;

/////////////////////////////////////////
///// Version with strong scrambler /////
/////////////////////////////////////////

static inline uint64_t get_bits_sc2_raw(MWC59State *obj)
{
    uint64_t x = obj->data ^ (obj->data << 4);
    x ^= x << 27;
    x = (x >> 27) & MWC59_MASK32;
    obj->data = MWC59_A * (obj->data & MWC59_MASK32) + (obj->data >> 32);
    return x;
}

MAKE_GET_BITS_WRAPPERS(sc2)

///////////////////////////////////////
///// Version with weak scrambler /////
///////////////////////////////////////

static inline uint64_t get_bits_sc1_raw(MWC59State *obj)
{
    const uint32_t x = (uint32_t) obj->data;
    obj->data = MWC59_A * (uint64_t)x + (obj->data >> 32);
    return x ^ (x << 8);
}

MAKE_GET_BITS_WRAPPERS(sc1)

/////////////////////////////////////
///// Version without scrambler /////
/////////////////////////////////////


static inline uint64_t get_bits_lin_raw(MWC59State *obj)
{
    const uint32_t x = (uint32_t) obj->data;
    obj->data = MWC59_A * (uint64_t)x + (obj->data >> 32);
    return x;
}

MAKE_GET_BITS_WRAPPERS(lin)

/////////////////////
///// Interface /////
/////////////////////

static void *create(const CallerAPI *intf)
{
    MWC59State *obj = intf->malloc(sizeof(MWC59State));
    obj->data = intf->get_seed64() & 0x3FFFFFFFFFFFFFF;
    if (obj->data == 0) {
        obj->data = 0x123456789;
    }
    return obj;
}

static const GeneratorParamVariant gen_list[] = {
    {"",    "mwc59:sc2", 32, default_create, get_bits_sc2, get_sum_sc2},
    {"sc2", "mwc59:sc2", 32, default_create, get_bits_sc2, get_sum_sc2},
    {"sc1", "mwc59:sc1", 32, default_create, get_bits_sc1, get_sum_sc1},
    {"lin", "mwc59:lin", 32, default_create, get_bits_lin, get_sum_lin},
    GENERATOR_PARAM_VARIANT_EMPTY
};

static const char description[] =
"mwc59 is the 59-bit multiply-with-carry generator that returns 32 bits.\n"
"The next param values are supported:\n"
"  sc2 - Strong scrambler (default version)\n"
"  sc1 - Weak scrambler\n"
"  lin - No scrambler, just returns the lower 32 bits of state.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}

#undef MWC59_A
#undef MWC59_MASK32
