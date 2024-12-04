/**
 * @file bat_default.c
 * @brief The `default` battery of tests that runs in about 5 minutes.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
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

static TestResults bspace32_1d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace32_1d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace64_1d(GeneratorState *obj)
{
    return bspace64_1d_ns_test(obj, 100);
}

static TestResults bspace32_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace32_2d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace8_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace4_8d_dec(GeneratorState *obj)
{
    return bspace4_8d_decimated_test(obj, 1 << 18);
}


///////////////////////////////
///// CollisionOver tests /////
///////////////////////////////

static TestResults collisionover8_5d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover8_5d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 5, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 5, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 5, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 5, .get_lower = 0};
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

static TestResults gap16_count0(GeneratorState *obj)
{
    return gap16_count0_test(obj, 500000000);
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

///////////////////////////////////
///// Linear complexity tests /////
///////////////////////////////////

static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, obj->gi->nbits - 1);
}

static TestResults linearcomp_mid(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, obj->gi->nbits / 2 - 1);
}


static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, 0);
}

///////////////////////////////////////
///// Hamming weights based tests /////
///////////////////////////////////////

static TestResults hamming_ot_all_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes, .nbytes = 1ull << 30};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_values_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_values, .nbytes = 1ull << 30};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_low1_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes_low1, .nbytes = 1ull << 30};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_low8_test(GeneratorState *obj)
{
    HammingOtOptions opts = {.mode = hamming_ot_bytes_low8, .nbytes = 1ull << 30};
    return hamming_ot_test(obj, &opts);
}

static TestResults hamming_ot_long128_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w128};
    return hamming_ot_long_test(obj, &opts);
}

static TestResults hamming_ot_long256_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w256};
    return hamming_ot_long_test(obj, &opts);
}

static TestResults hamming_ot_long512_test(GeneratorState *obj)
{
    HammingOtLongOptions opts = {.nvalues = 1ull << 30, .wordsize = hamming_ot_w512};
    return hamming_ot_long_test(obj, &opts);
}

///////////////////////
///// Other tests /////
///////////////////////

static TestResults mod3_short_test(GeneratorState *obj)
{
    return mod3_test(obj, 1ull << 28);
}


void battery_default(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    static const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test, 1, ram_lo},
        {"byte_freq", byte_freq_test, 1, ram_med},
        {"word16_freq", word16_freq_test, 17, ram_med},
        {"bspace64_1d", bspace64_1d, 34, ram_hi},
        {"bspace32_1d", bspace32_1d, 2, ram_hi},
        {"bspace32_1d_high", bspace32_1d_high, 2, ram_hi},
        {"bspace32_2d", bspace32_2d, 3, ram_hi},
        {"bspace32_2d_high", bspace32_2d_high, 3, ram_hi},
        {"bspace21_3d", bspace21_3d, 3, ram_hi},
        {"bspace21_3d_high", bspace21_3d_high, 3, ram_hi},
        {"bspace16_4d", bspace16_4d, 4, ram_hi},
        {"bspace16_4d_high", bspace16_4d_high, 4, ram_hi},
        {"bspace8_8d", bspace8_8d, 4, ram_med},
        {"bspace8_8d_high", bspace8_8d_high, 4, ram_med},
        {"bspace4_8d_dec", bspace4_8d_dec, 30, ram_lo},
        {"bspace4_16d", bspace4_16d, 6, ram_med},
        {"bspace4_16d_high", bspace4_16d_high, 6, ram_med},
        {"collover20_2d", collisionover20_2d, 7, ram_hi},
        {"collover20_2d_high", collisionover20_2d_high, 7, ram_hi},
        {"collover13_3d", collisionover13_3d, 8, ram_hi},
        {"collover13_3d_high", collisionover13_3d_high, 8, ram_hi},
        {"collover8_5d", collisionover8_5d, 7, ram_med},
        {"collover8_5d_high", collisionover8_5d_high, 7, ram_med},
        {"collover5_8d", collisionover5_8d, 7, ram_med},
        {"collover5_8d_high", collisionover5_8d_high, 7, ram_med},
        {"gap_inv512", gap_inv512, 14, ram_lo},
        {"gap16_count0", gap16_count0, 10, ram_med},
        {"hamming_ot", hamming_ot_all_test, 5, ram_med},
        {"hamming_ot_low1", hamming_ot_low1_test, 5, ram_med},
        {"hamming_ot_low8", hamming_ot_low8_test, 5, ram_med},
        {"hamming_ot_values", hamming_ot_values_test, 5, ram_med},
        {"hamming_ot_u128", hamming_ot_long128_test, 7, ram_med},
        {"hamming_ot_u256", hamming_ot_long256_test, 7, ram_med},
        {"hamming_ot_u512", hamming_ot_long512_test, 7, ram_med},
        {"linearcomp_high", linearcomp_high, 1, ram_lo},
        {"linearcomp_mid", linearcomp_mid, 1, ram_lo},
        {"linearcomp_low", linearcomp_low, 1, ram_lo},
        {"matrixrank_4096", matrixrank_4096, 4, ram_med},
        {"matrixrank_4096_low8", matrixrank_4096_low8, 5, ram_med},
        {"mod3", mod3_short_test, 1, ram_med},
        {NULL, NULL, 0, 0}
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
