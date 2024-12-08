/**
 * @file bat_full.c
 * @brief The `full` battery of tests that runs in about 1 hour.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
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

/**
 * @brief One-dimensional 32-bit birthday spacings test. Allows to catch additive lagged 
 * Fibonacci PRNGs. In the case of 32-bit PRNG it is equivalent to `bspace32_1d'.
 */
static TestResults bspace32_1d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

/**
 * @brief One-dimensional 32-bit birthday spacings test. Allows to catch additive lagged 
 * Fibonacci PRNGs. In the case of 32-bit PRNG it is equivalent to `bspace32_1d'.
 */
static TestResults bspace32_1d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace64_1d(GeneratorState *obj)
{
    return bspace64_1d_ns_test(obj, 250);
}


static TestResults bspace32_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 250, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace32_2d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 250, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace21_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 200, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 200, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 200, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 200, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 200, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 200, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 200, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 200, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_8d_dec(GeneratorState *obj)
{
    return bspace4_8d_decimated_test(obj, 262144);
}


///////////////////////////////
///// CollisionOver tests /////
///////////////////////////////

static TestResults collisionover8_5d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 50, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover8_5d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 50, .get_lower = 0};
    return collisionover_test(obj, &opts);
}


static TestResults collisionover5_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 50, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 50, .get_lower = 0};
    return collisionover_test(obj, &opts);
}


static TestResults collisionover13_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 50, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 50, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 50, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 50, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

/////////////////////
///// Gap tests /////
/////////////////////

static TestResults gap_inv512(GeneratorState *obj)
{
    GapOptions opts = {.shl = 9, .ngaps = 10000000};
    return gap_test(obj, &opts);
}

/**
 * @brief This modification of gap test allows to catch some additive/subtractive
 * lagged Fibonacci PRNGs with big lags. Also detects ChaCha12 with 32-bit
 * counter: the test consumes more than 2^36 values.
 */
static TestResults gap_inv1024(GeneratorState *obj)
{
    GapOptions opts = {.shl = 10, .ngaps = 100000000};
    return gap_test(obj, &opts);
}


static TestResults gap16_count0(GeneratorState *obj)
{
    return gap16_count0_test(obj, 1000000000);
}

/////////////////////////////
///// Matrix rank tests /////
/////////////////////////////

static TestResults matrixrank_4096(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 64);
}

static TestResults matrixrank_4096_low8(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 8);
}

static TestResults matrixrank_8192(GeneratorState *obj)
{
    return matrixrank_test(obj, 8192, 64);
}

static TestResults matrixrank_8192_low8(GeneratorState *obj)
{
    return matrixrank_test(obj, 8192, 8);
}

///////////////////////////////////
///// Linear complexity tests /////
///////////////////////////////////

static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 500000, obj->gi->nbits - 1);
}

static TestResults linearcomp_mid(GeneratorState *obj)
{
    return linearcomp_test(obj, 500000, obj->gi->nbits / 2 - 1);
}


static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 500000, 0);
}

///////////////////////////////////////
///// Hamming weights based tests /////
///////////////////////////////////////

static TestResults hamming_ot_all_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes, .nbytes = 1ull << 33};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_values_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_values, .nbytes = 1ull << 33};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_low1_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes_low1, .nbytes = 1ull << 32};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_low8_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes_low8, .nbytes = 1ull << 32};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_long128_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w128};
    return hamming_ot_long_test(obj, &opts);
}

static TestResults hamming_ot_long256_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w256};
    return hamming_ot_long_test(obj, &opts);
}

static TestResults hamming_ot_long512_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 33, .wordsize = hamming_ot_w512};
    return hamming_ot_long_test(obj, &opts);
}

///////////////////////
///// Other tests /////
///////////////////////

static TestResults mod3_long_test(GeneratorState *obj)
{
    return mod3_test(obj, 1ull << 30);
}


static TestResults sumcollector_long_test(GeneratorState *obj)
{
    return sumcollector_test(obj, 20000000000);
}


void battery_full(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    static const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test, 1, ram_lo},
        {"byte_freq", byte_freq_test, 1, ram_med},
        {"word16_freq", word16_freq_test, 20, ram_med},
        {"bspace64_1d", bspace64_1d, 90, ram_hi},
        {"bspace32_1d", bspace32_1d, 2, ram_hi},
        {"bspace32_1d_high", bspace32_1d_high, 2, ram_hi},
        {"bspace32_2d", bspace32_2d, 90, ram_hi},
        {"bspace32_2d_high", bspace32_2d_high, 90, ram_hi},
        {"bspace21_3d", bspace21_3d, 62, ram_hi},
        {"bspace21_3d_high", bspace21_3d_high, 62, ram_hi},
        {"bspace16_4d", bspace16_4d, 78, ram_med},
        {"bspace16_4d_high", bspace16_4d_high, 78, ram_med},
        {"bspace8_8d", bspace8_8d, 96, ram_med},
        {"bspace8_8d_high", bspace8_8d_high, 96, ram_med},
        {"bspace4_8d_dec", bspace4_8d_dec, 30, ram_lo},
        {"bspace4_16d", bspace4_16d, 116, ram_lo},
        {"bspace4_16d_high", bspace4_16d_high, 116, ram_lo},
        {"collover20_2d", collisionover20_2d, 73, ram_hi},
        {"collover20_2d_high", collisionover20_2d_high, 73, ram_hi},
        {"collover13_3d", collisionover13_3d, 73, ram_hi},
        {"collover13_3d_high", collisionover13_3d_high, 73, ram_hi},
        {"collover8_5d", collisionover8_5d, 73, ram_med},
        {"collover8_5d_high", collisionover8_5d_high, 73, ram_med},
        {"collover5_8d", collisionover5_8d, 72, ram_med},
        {"collover5_8d_high", collisionover5_8d_high, 72, ram_med},
        {"gap_inv512", gap_inv512, 14, ram_lo},
        {"gap_inv1024", gap_inv1024, 284, ram_lo},
        {"gap16_count0", gap16_count0, 27, ram_med},
        {"hamming_ot", hamming_ot_all_test, 36, ram_med},
        {"hamming_ot_long128", hamming_ot_long128_test, 55, ram_med},
        {"hamming_ot_long256", hamming_ot_long256_test, 55, ram_med},
        {"hamming_ot_long512", hamming_ot_long512_test, 55, ram_med},
        {"hamming_ot_low1", hamming_ot_low1_test, 4, ram_med},
        {"hamming_ot_low8", hamming_ot_low8_test, 8, ram_med},
        {"hamming_ot_values", hamming_ot_values_test, 16, ram_med},
        {"linearcomp_high", linearcomp_high, 35, ram_lo},
        {"linearcomp_mid", linearcomp_mid, 35, ram_lo},
        {"linearcomp_low", linearcomp_low, 35, ram_lo},
        {"matrixrank_4096", matrixrank_4096, 4, ram_med},
        {"matrixrank_4096_low8", matrixrank_4096_low8, 5, ram_med},
        {"matrixrank_8192", matrixrank_8192, 36, ram_med},
        {"matrixrank_8192_low8", matrixrank_8192_low8, 36, ram_med},
        {"mod3", mod3_long_test, 1, ram_med},
        {"sumcollector", sumcollector_long_test, 60, ram_med},
        {NULL, NULL, 0, 0}
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
