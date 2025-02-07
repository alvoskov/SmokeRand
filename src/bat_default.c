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

///////////////////////////////////
///// Birthday spacings tests /////
///////////////////////////////////

static TestResults bspace64_1d(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return bspace64_1d_ns_test(obj, 100);
}

static TestResults bspace4_8d_dec(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return bspace4_8d_decimated_test(obj, 1 << 18);
}


///////////////////////
///// Other tests /////
///////////////////////

static TestResults mod3_short_test(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return mod3_test(obj, 1ull << 28);
}


void battery_default(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    // Birthday spacings tests options
    static const BSpaceNDOptions
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

    // CollisionOver tests options
    static const BSpaceNDOptions
        collover8_5d       = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 5, .get_lower = 1},
        collover8_5d_high  = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 5, .get_lower = 0},
        collover5_8d       = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 5, .get_lower = 1},
        collover5_8d_high  = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 5, .get_lower = 0},
        collover13_3d      = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 5, .get_lower = 1},
        collover13_3d_high = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 5, .get_lower = 0},
        collover20_2d      = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 5, .get_lower = 1},
        collover20_2d_high = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 5, .get_lower = 0};

    // Gap test
    static const GapOptions gap_inv512 = {.shl = 9, .ngaps = 10000000};
    static const Gap16Count0Options gap16_count0 = {.ngaps = 500000000};

    // Hamming weights based tests
    static const HammingOtOptions
        hw_ot_all    = {.mode = hamming_ot_bytes,      .nbytes = 1ull << 30},
        hw_ot_values = {.mode = hamming_ot_values,     .nbytes = 1ull << 30},
        hw_ot_low1   = {.mode = hamming_ot_bytes_low1, .nbytes = 1ull << 30},
        hw_ot_low8   = {.mode = hamming_ot_bytes_low8, .nbytes = 1ull << 30};

    // Hamming weights based "long" tests
    static const HammingOtLongOptions
        hw_ot_long128 = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w128},
        hw_ot_long256 = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w256},
        hw_ot_long512 = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w512};

    // Linear complexity tests
    static const LinearCompOptions
        linearcomp_low  = {.nbits = 100000, .bitpos = linearcomp_bitpos_low},
        linearcomp_mid  = {.nbits = 100000, .bitpos = linearcomp_bitpos_mid},
        linearcomp_high = {.nbits = 100000, .bitpos = linearcomp_bitpos_high};

    // Matrix rank tests
    static const MatrixRankOptions
        matrixrank_4096      = {.n = 4096, .max_nbits = 64},
        matrixrank_4096_low8 = {.n = 4096, .max_nbits = 8};

    static const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test_wrap, NULL, 1, ram_lo},
        {"byte_freq", byte_freq_test_wrap, NULL, 1, ram_med},
        {"word16_freq", word16_freq_test_wrap, NULL, 17, ram_med},
        {"bspace64_1d", bspace64_1d, NULL, 34, ram_hi},
        {"bspace32_1d",      bspace_nd_test_wrap, &bspace32_1d,      2, ram_hi},
        {"bspace32_1d_high", bspace_nd_test_wrap, &bspace32_1d_high, 2, ram_hi},
        {"bspace32_2d",      bspace_nd_test_wrap, &bspace32_2d,      3, ram_hi},
        {"bspace32_2d_high", bspace_nd_test_wrap, &bspace32_2d_high, 3, ram_hi},
        {"bspace21_3d",      bspace_nd_test_wrap, &bspace21_3d,      3, ram_hi},
        {"bspace21_3d_high", bspace_nd_test_wrap, &bspace21_3d_high, 3, ram_hi},
        {"bspace16_4d",      bspace_nd_test_wrap, &bspace16_4d,      4, ram_hi},
        {"bspace16_4d_high", bspace_nd_test_wrap, &bspace16_4d_high, 4, ram_hi},
        {"bspace8_8d",       bspace_nd_test_wrap, &bspace8_8d,       4, ram_med},
        {"bspace8_8d_high",  bspace_nd_test_wrap, &bspace8_8d_high,  4, ram_med},
        {"bspace4_8d_dec",   bspace4_8d_dec, NULL, 30, ram_lo},
        {"bspace4_16d",      bspace_nd_test_wrap, &bspace4_16d,      6, ram_med},
        {"bspace4_16d_high", bspace_nd_test_wrap, &bspace4_16d_high, 6, ram_med},
        {"collover20_2d",      collisionover_test_wrap, &collover20_2d,      7, ram_hi},
        {"collover20_2d_high", collisionover_test_wrap, &collover20_2d_high, 7, ram_hi},
        {"collover13_3d",      collisionover_test_wrap, &collover13_3d,      8, ram_hi},
        {"collover13_3d_high", collisionover_test_wrap, &collover13_3d_high, 8, ram_hi},
        {"collover8_5d",       collisionover_test_wrap, &collover8_5d,       7, ram_med},
        {"collover8_5d_high",  collisionover_test_wrap, &collover8_5d_high,  7, ram_med},
        {"collover5_8d",       collisionover_test_wrap, &collover5_8d,       7, ram_med},
        {"collover5_8d_high",  collisionover_test_wrap, &collover5_8d_high,  7, ram_med},
        {"gap_inv512", gap_test_wrap, &gap_inv512, 14, ram_lo},
        {"gap16_count0", gap16_count0_test_wrap, &gap16_count0, 10, ram_med},
        {"hamming_ot",        hamming_ot_test_wrap, &hw_ot_all,  5, ram_med},
        {"hamming_ot_low1",   hamming_ot_test_wrap, &hw_ot_low1, 5, ram_med},
        {"hamming_ot_low8",   hamming_ot_test_wrap, &hw_ot_low8, 5, ram_med},
        {"hamming_ot_values", hamming_ot_test_wrap, &hw_ot_values, 5, ram_med},
        {"hamming_ot_u128",   hamming_ot_long_test_wrap, &hw_ot_long128, 7, ram_med},
        {"hamming_ot_u256",   hamming_ot_long_test_wrap, &hw_ot_long256, 7, ram_med},
        {"hamming_ot_u512",   hamming_ot_long_test_wrap, &hw_ot_long512, 7, ram_med},
        {"linearcomp_high", linearcomp_test_wrap, &linearcomp_high, 1, ram_lo},
        {"linearcomp_mid",  linearcomp_test_wrap, &linearcomp_mid,  1, ram_lo},
        {"linearcomp_low",  linearcomp_test_wrap, &linearcomp_low,  1, ram_lo},
        {"matrixrank_4096", matrixrank_test_wrap, &matrixrank_4096, 4, ram_med},
        {"matrixrank_4096_low8", matrixrank_test_wrap, &matrixrank_4096_low8, 5, ram_med},
        {"mod3", mod3_short_test, NULL, 1, ram_med},
        {NULL, NULL, NULL, 0, 0}
    };

    const TestsBattery bat = {
        "default", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }
}
