/**
 * @file bat_full.c
 * @brief The `full` battery of tests that runs in about 1 hour.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_full.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/hwtests.h"
#include "smokerand/entropy.h"

///////////////////////////////////
///// Birthday spacings tests /////
///////////////////////////////////

static TestResults bspace64_1d(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return bspace64_1d_ns_test(obj, 250);
}


static TestResults bspace4_8d_dec(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return bspace4_8d_decimated_test(obj, 262144);
}


///////////////////////
///// Other tests /////
///////////////////////

static TestResults mod3_long_test(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return mod3_test(obj, 1ull << 30);
}


static TestResults sumcollector_long_test(GeneratorState *obj, const void *udata)
{
    (void) udata;
    return sumcollector_test(obj, 20000000000);
}


/**
 * @details One-dimensional 32-bit birthday spacings test. Allows to catch additive lagged 
 * Fibonacci PRNGs. In the case of 32-bit PRNG it is equivalent to `bspace32_1d'.
 *
 * The gap_inv1024 modification of gap test allows to catch some additive/subtractive
 * lagged Fibonacci PRNGs with big lags. Also detects ChaCha12 with 32-bit
 * counter: the test consumes more than 2^36 values.
 */
void battery_full(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    // Birthday spacings tests options
    static const BSpaceNDOptions
        bspace32_1d      = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 1},
        bspace32_1d_high = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 0},
        bspace32_2d      = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 250, .get_lower = 1},
        bspace32_2d_high = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 250, .get_lower = 0},
        bspace21_3d      = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 200, .get_lower = 1},
        bspace21_3d_high = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 200, .get_lower = 0},
        bspace16_4d      = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 200, .get_lower = 1},
        bspace16_4d_high = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 200, .get_lower = 0},
        bspace8_8d       = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 200, .get_lower = 1},
        bspace8_8d_high  = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 200, .get_lower = 0},
        bspace4_16d      = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 200, .get_lower = 1},
        bspace4_16d_high = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 200, .get_lower = 0};

    // CollisionOver tests options
    static const BSpaceNDOptions
        collover8_5d       = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 50, .get_lower = 1},
        collover8_5d_high  = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 50, .get_lower = 0},
        collover5_8d       = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 50, .get_lower = 1},
        collover5_8d_high  = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 50, .get_lower = 0},
        collover13_3d      = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 50, .get_lower = 1},
        collover13_3d_high = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 50, .get_lower = 0},
        collover20_2d      = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 50, .get_lower = 1},
        collover20_2d_high = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 50, .get_lower = 0};

    // Gap test
    static const GapOptions gap_inv512  = {.shl = 9,  .ngaps = 10000000};
    static const GapOptions gap_inv1024 = {.shl = 10, .ngaps = 100000000};
    static const Gap16Count0Options gap16_count0 = {.ngaps = 1000000000};

    // Hamming weights based tests
    static const HammingOtOptions
        hw_ot_all    = {.mode = hamming_ot_bytes, .nbytes = 1ull << 33},
        hw_ot_values = {.mode = hamming_ot_values, .nbytes = 1ull << 33},
        hw_ot_low1   = {.mode = hamming_ot_bytes_low1, .nbytes = 1ull << 33},
        hw_ot_low8   = {.mode = hamming_ot_bytes_low8, .nbytes = 1ull << 33};

    // Hamming weights based "long" tests
    static const HammingOtLongOptions
        hw_ot_long128 = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w128},
        hw_ot_long256 = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w256},
        hw_ot_long512 = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w512};

    // Linear complexity tests
    static const LinearCompOptions
        linearcomp_low  = {.nbits = 500000, .bitpos = linearcomp_bitpos_low},
        linearcomp_mid  = {.nbits = 500000, .bitpos = linearcomp_bitpos_mid},
        linearcomp_high = {.nbits = 500000, .bitpos = linearcomp_bitpos_high};

    // Matrix rank tests
    static const MatrixRankOptions
        matrixrank_4096      = {.n = 4096, .max_nbits = 64},
        matrixrank_4096_low8 = {.n = 4096, .max_nbits = 8},
        matrixrank_8192      = {.n = 8192, .max_nbits = 64},
        matrixrank_8192_low8 = {.n = 8192, .max_nbits = 8};


    static const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test_wrap, NULL, 1, ram_lo},
        {"byte_freq", byte_freq_test_wrap, NULL, 1, ram_med},
        {"word16_freq", word16_freq_test_wrap, NULL, 20, ram_med},
        {"bspace64_1d", bspace64_1d, NULL, 90, ram_hi},
        {"bspace32_1d",      bspace_nd_test_wrap, &bspace32_1d,      2, ram_hi},
        {"bspace32_1d_high", bspace_nd_test_wrap, &bspace32_1d_high, 2, ram_hi},
        {"bspace32_2d",      bspace_nd_test_wrap, &bspace32_2d,      90, ram_hi},
        {"bspace32_2d_high", bspace_nd_test_wrap, &bspace32_2d_high, 90, ram_hi},
        {"bspace21_3d",      bspace_nd_test_wrap, &bspace21_3d,      62, ram_hi},
        {"bspace21_3d_high", bspace_nd_test_wrap, &bspace21_3d_high, 62, ram_hi},
        {"bspace16_4d",      bspace_nd_test_wrap, &bspace16_4d,      78, ram_med},
        {"bspace16_4d_high", bspace_nd_test_wrap, &bspace16_4d_high, 78, ram_med},
        {"bspace8_8d",       bspace_nd_test_wrap, &bspace8_8d,       96, ram_med},
        {"bspace8_8d_high",  bspace_nd_test_wrap, &bspace8_8d_high,  96, ram_med},
        {"bspace4_8d_dec",   bspace4_8d_dec, NULL, 30, ram_lo},
        {"bspace4_16d",      bspace_nd_test_wrap, &bspace4_16d,      116, ram_lo},
        {"bspace4_16d_high", bspace_nd_test_wrap, &bspace4_16d_high, 116, ram_lo},
        {"collover20_2d",      collisionover_test_wrap, &collover20_2d,      73, ram_hi},
        {"collover20_2d_high", collisionover_test_wrap, &collover20_2d_high, 73, ram_hi},
        {"collover13_3d",      collisionover_test_wrap, &collover13_3d,      73, ram_hi},
        {"collover13_3d_high", collisionover_test_wrap, &collover13_3d_high, 73, ram_hi},
        {"collover8_5d",       collisionover_test_wrap, &collover8_5d,       73, ram_med},
        {"collover8_5d_high",  collisionover_test_wrap, &collover8_5d_high,  73, ram_med},
        {"collover5_8d",       collisionover_test_wrap, &collover5_8d,       72, ram_med},
        {"collover5_8d_high",  collisionover_test_wrap, &collover5_8d_high,  72, ram_med},
        {"gap_inv512",  gap_test_wrap, &gap_inv512, 14, ram_lo},
        {"gap_inv1024", gap_test_wrap, &gap_inv1024, 284, ram_lo},
        {"gap16_count0", gap16_count0_test_wrap, &gap16_count0, 27, ram_med},
        {"hamming_ot",        hamming_ot_test_wrap, &hw_ot_all,    36, ram_med},
        {"hamming_ot_low1",   hamming_ot_test_wrap, &hw_ot_low1,   8, ram_med},
        {"hamming_ot_low8",   hamming_ot_test_wrap, &hw_ot_low8,   16, ram_med},
        {"hamming_ot_values", hamming_ot_test_wrap, &hw_ot_values, 16, ram_med},
        {"hamming_ot_u128",   hamming_ot_long_test_wrap, &hw_ot_long128, 55, ram_med},
        {"hamming_ot_u256",   hamming_ot_long_test_wrap, &hw_ot_long256, 55, ram_med},
        {"hamming_ot_u512",   hamming_ot_long_test_wrap, &hw_ot_long512, 55, ram_med},
        {"linearcomp_high",   linearcomp_test_wrap, &linearcomp_high, 35, ram_lo},
        {"linearcomp_mid",    linearcomp_test_wrap, &linearcomp_mid,  35, ram_lo},
        {"linearcomp_low",    linearcomp_test_wrap, &linearcomp_low,  35, ram_lo},
        {"matrixrank_4096",      matrixrank_test_wrap, &matrixrank_4096, 4, ram_med},
        {"matrixrank_4096_low8", matrixrank_test_wrap, &matrixrank_4096_low8, 5, ram_med},
        {"matrixrank_8192",      matrixrank_test_wrap, &matrixrank_8192, 36, ram_med},
        {"matrixrank_8192_low8", matrixrank_test_wrap, &matrixrank_8192_low8, 36, ram_med},
        {"mod3", mod3_long_test, NULL, 1, ram_med},
        {"sumcollector", sumcollector_long_test, NULL, 60, ram_med},
        {NULL, NULL, NULL, 0, 0}
    };

    const TestsBattery bat = {
        "full", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }
}
