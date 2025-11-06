/**
 * @file bat_default.c
 * @brief The `default` battery of tests that runs in about 5 minutes.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_default.h"
#include "smokerand/coretests.h"
#include "smokerand/hwtests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"

#define COLLOVER_LO_PROPS .nsamples = 5, .n = COLLOVER_DEFAULT_N, .get_lower = 1
#define COLLOVER_HI_PROPS .nsamples = 5, .n = COLLOVER_DEFAULT_N, .get_lower = 0

/**
 * @brief SmokeRand `default` battery.
 */
BatteryExitCode battery_default(const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype)
{
    // Monobit frequency test options
    static const MonobitFreqOptions monobit = {.nvalues = 1ull << 28};
    // Birthday spacings tests options
    static const BSpaceNDOptions
        bspace64_1d      = {.nbits_per_dim = 64, .ndims = 1,  .nsamples = 100,  .get_lower = 1},
        bspace32_1d      = {.nbits_per_dim = 32, .ndims = 1,  .nsamples = 8192, .get_lower = 1},
        bspace32_1d_high = {.nbits_per_dim = 32, .ndims = 1,  .nsamples = 8192, .get_lower = 0},
        bspace32_2d      = {.nbits_per_dim = 32, .ndims = 2,  .nsamples = 10,   .get_lower = 1},
        bspace32_2d_high = {.nbits_per_dim = 32, .ndims = 2,  .nsamples = 10,   .get_lower = 0},
        bspace21_3d      = {.nbits_per_dim = 21, .ndims = 3,  .nsamples = 10,   .get_lower = 1},
        bspace21_3d_high = {.nbits_per_dim = 21, .ndims = 3,  .nsamples = 10,   .get_lower = 0},
        bspace16_4d      = {.nbits_per_dim = 16, .ndims = 4,  .nsamples = 10,   .get_lower = 1},
        bspace16_4d_high = {.nbits_per_dim = 16, .ndims = 4,  .nsamples = 10,   .get_lower = 0},
        bspace8_8d       = {.nbits_per_dim = 8,  .ndims = 8,  .nsamples = 10,   .get_lower = 1},
        bspace8_8d_high  = {.nbits_per_dim = 8,  .ndims = 8,  .nsamples = 10,   .get_lower = 0},
        bspace4_16d      = {.nbits_per_dim = 4,  .ndims = 16, .nsamples = 10,   .get_lower = 1},
        bspace4_16d_high = {.nbits_per_dim = 4,  .ndims = 16, .nsamples = 10,   .get_lower = 0};

    // Birthday spacings test with decimation
    static const BSpace4x8dDecimatedOptions bs_dec = {.step = 1 << 18};

    // CollisionOver tests options
    static const CollOverNDOptions
        collover8_5d       = {.nbits_per_dim = 8,  .ndims = 5, COLLOVER_LO_PROPS},
        collover8_5d_high  = {.nbits_per_dim = 8,  .ndims = 5, COLLOVER_HI_PROPS},
        collover5_8d       = {.nbits_per_dim = 5,  .ndims = 8, COLLOVER_LO_PROPS},
        collover5_8d_high  = {.nbits_per_dim = 5,  .ndims = 8, COLLOVER_HI_PROPS},
        collover13_3d      = {.nbits_per_dim = 13, .ndims = 3, COLLOVER_LO_PROPS},
        collover13_3d_high = {.nbits_per_dim = 13, .ndims = 3, COLLOVER_HI_PROPS},
        collover20_2d      = {.nbits_per_dim = 20, .ndims = 2, COLLOVER_LO_PROPS},
        collover20_2d_high = {.nbits_per_dim = 20, .ndims = 2, COLLOVER_HI_PROPS};

    // Gap test
    static const GapOptions
        gap_inv8   = {.shl = 3, .ngaps = 500000000},
        gap_inv512 = {.shl = 9, .ngaps = 10000000};
    static const Gap16Count0Options gap16_count0 = {.ngaps = 500000000};

    // Hamming weights distribution (histogram) test
    static const HammingDistrOptions
        hw_distr = {.nvalues = 1ull << 30, .nlevels = 10};

    // Hamming weights based tests
    static const HammingOtOptions
        hw_ot_all    = {.mode = HAMMING_OT_BYTES,      .nbytes = 1ull << 30},
        hw_ot_values = {.mode = HAMMING_OT_VALUES,     .nbytes = 1ull << 30},
        hw_ot_low1   = {.mode = HAMMING_OT_BYTES_LOW1, .nbytes = 1ull << 30},
        hw_ot_low8   = {.mode = HAMMING_OT_BYTES_LOW8, .nbytes = 1ull << 30};

    // Hamming weights based "long" tests
    static const HammingOtLongOptions
        hw_ot_long128 = {.nvalues = 1ull << 30, .wordsize = HAMMING_OT_W128},
        hw_ot_long256 = {.nvalues = 1ull << 30, .wordsize = HAMMING_OT_W256},
        hw_ot_long512 = {.nvalues = 1ull << 30, .wordsize = HAMMING_OT_W512};

    // Linear complexity tests
    static const LinearCompOptions
        linearcomp_low  = {.nbits = 100000, .bitpos = LINEARCOMP_BITPOS_LOW},
        linearcomp_mid  = {.nbits = 100000, .bitpos = LINEARCOMP_BITPOS_MID},
        linearcomp_high = {.nbits = 100000, .bitpos = LINEARCOMP_BITPOS_HIGH};

    // Matrix rank tests
    static const MatrixRankOptions
        matrixrank_4096      = {.n = 4096, .max_nbits = 64},
        matrixrank_4096_low8 = {.n = 4096, .max_nbits = 8};

    // mod3 test
    static const Mod3Options mod3 = {.nvalues = 1ull << 28};

    static const TestDescription tests[] = {
        {"monobit_freq",         monobit_freq_test_wrap, &monobit},
        {"byte_freq",            byte_freq_test_wrap, NULL},
        {"word16_freq",          word16_freq_test_wrap, NULL},
        {"bspace64_1d",          bspace_nd_test_wrap, &bspace64_1d},
        {"bspace32_1d",          bspace_nd_test_wrap, &bspace32_1d},
        {"bspace32_1d_high",     bspace_nd_test_wrap, &bspace32_1d_high},
        {"bspace32_2d",          bspace_nd_test_wrap, &bspace32_2d},
        {"bspace32_2d_high",     bspace_nd_test_wrap, &bspace32_2d_high},
        {"bspace21_3d",          bspace_nd_test_wrap, &bspace21_3d},
        {"bspace21_3d_high",     bspace_nd_test_wrap, &bspace21_3d_high},
        {"bspace16_4d",          bspace_nd_test_wrap, &bspace16_4d},
        {"bspace16_4d_high",     bspace_nd_test_wrap, &bspace16_4d_high},
        {"bspace8_8d",           bspace_nd_test_wrap, &bspace8_8d},
        {"bspace8_8d_high",      bspace_nd_test_wrap, &bspace8_8d_high},
        {"bspace4_8d_dec",       bspace4_8d_decimated_test_wrap, &bs_dec},
        {"bspace4_16d",          bspace_nd_test_wrap, &bspace4_16d},
        {"bspace4_16d_high",     bspace_nd_test_wrap, &bspace4_16d_high},
        {"collover20_2d",        collisionover_test_wrap, &collover20_2d},
        {"collover20_2d_high",   collisionover_test_wrap, &collover20_2d_high},
        {"collover13_3d",        collisionover_test_wrap, &collover13_3d},
        {"collover13_3d_high",   collisionover_test_wrap, &collover13_3d_high},
        {"collover8_5d",         collisionover_test_wrap, &collover8_5d},
        {"collover8_5d_high",    collisionover_test_wrap, &collover8_5d_high},
        {"collover5_8d",         collisionover_test_wrap, &collover5_8d},
        {"collover5_8d_high",    collisionover_test_wrap, &collover5_8d_high},
        {"gap_inv8",             gap_test_wrap, &gap_inv8},
        {"gap_inv512",           gap_test_wrap, &gap_inv512},
        {"gap16_count0",         gap16_count0_test_wrap, &gap16_count0},
        {"hamming_distr",        hamming_distr_test_wrap, &hw_distr},
        {"hamming_ot",           hamming_ot_test_wrap, &hw_ot_all},
        {"hamming_ot_low1",      hamming_ot_test_wrap, &hw_ot_low1},
        {"hamming_ot_low8",      hamming_ot_test_wrap, &hw_ot_low8},
        {"hamming_ot_values",    hamming_ot_test_wrap, &hw_ot_values},
        {"hamming_ot_u128",      hamming_ot_long_test_wrap, &hw_ot_long128},
        {"hamming_ot_u256",      hamming_ot_long_test_wrap, &hw_ot_long256},
        {"hamming_ot_u512",      hamming_ot_long_test_wrap, &hw_ot_long512},
        {"linearcomp_high",      linearcomp_test_wrap, &linearcomp_high},
        {"linearcomp_mid",       linearcomp_test_wrap, &linearcomp_mid},
        {"linearcomp_low",       linearcomp_test_wrap, &linearcomp_low},
        {"matrixrank_4096",      matrixrank_test_wrap, &matrixrank_4096},
        {"matrixrank_4096_low8", matrixrank_test_wrap, &matrixrank_4096_low8},
        {"mod3", mod3_test_wrap, &mod3},
        {NULL, NULL, NULL}
    };

    const TestsBattery bat = {
        "default", tests
    };
    if (gen != NULL) {
        return TestsBattery_run(&bat, gen, intf, testid, nthreads, rtype);
    } else {
        TestsBattery_print_info(&bat);
        return BATTERY_PASSED;
    }
}
