/**
 * @file bat_brief.c
 * @brief The `brief` battery of tests that runs in about 1 minute.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_default.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"

///////////////////////////////////
///// Birthday spacings tests /////
///////////////////////////////////

static TestResults bspace32_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 4096, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace32_1d_hi_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 4096, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


typedef struct {
    const GeneratorInfo *gen32;
    void *state;
} Bits64From32State;

static uint64_t get_bits64_from32(void *state)
{
    Bits64From32State *obj = state;
    uint64_t x = obj->gen32->get_bits(obj->state);
    uint64_t y = obj->gen32->get_bits(obj->state);
    return (x << 32) | y;
}


static TestResults bspace64_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 64, .ndims = 1, .nsamples = 40, .get_lower = 1};
    if (obj->gi->nbits == 64) {
        return bspace_nd_test(obj, &opts);
    } else {
        GeneratorInfo gen32dup = *(obj->gi);
        gen32dup.get_bits = get_bits64_from32;
        Bits64From32State statedup = {obj->gi, obj->state};
        GeneratorState objdup = {.gi = &gen32dup, .state = &statedup, .intf = obj->intf};
        return bspace_nd_test(&objdup, &opts);
    }
}


static TestResults bspace32_2d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 5, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 5, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 5, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 5, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

///////////////////////////////
///// CollisionOver tests /////
///////////////////////////////

static TestResults collisionover8_5d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .nsamples = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .nsamples = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .nsamples = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .nsamples = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

/////////////////////
///// Gap tests /////
/////////////////////

/**
 * @brief A modification of gap tests that consumes more than \f$ 2^32 \f$
 * values and allows to detect any PRNG with 32-bit state. It also detects
 * additive/subtractive lagged Fibonacci and AWC/SWB generators.
 */
static TestResults gap_inv512(GeneratorState *obj)
{
    return gap_test(obj, &(GapOptions) {.shl = 9, .ngaps = 10000000});
}

///////////////////////////////////
///// Linear complexity tests /////
///////////////////////////////////

static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 50000, obj->gi->nbits - 1);
}

static TestResults linearcomp_mid(GeneratorState *obj)
{
    return linearcomp_test(obj, 50000, obj->gi->nbits / 2 - 1);
}


static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 50000, 0);
}


///////////////////////////////////////
///// Hamming weights based tests /////
///////////////////////////////////////

static TestResults hamming_dc6_values_test(GeneratorState *obj)
{
    HammingDc6Options opts = {.mode = hamming_dc6_values, .nbytes = 1ull << 28};
    return hamming_dc6_test(obj, &opts);
}

static TestResults hamming_dc6_low1_test(GeneratorState *obj)
{
    HammingDc6Options opts = {.mode = hamming_dc6_bytes_low1, .nbytes = 1ull << 30};
    return hamming_dc6_test(obj, &opts);
}


void battery_brief(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test, 2},
        {"byte_freq", byte_freq_test, 2},
        {"bspace64_1d", bspace64_1d_test, 23},
        {"bspace32_1d", bspace32_1d_test, 2},
        {"bspace32_1d_high", bspace32_1d_hi_test, 2},
        {"bspace32_2d", bspace32_2d_test, 3},
        {"bspace21_3d", bspace21_3d_test, 2},
        {"bspace16_4d", bspace16_4d_test, 3},
        {"bspace8_8d", bspace8_8d_test, 3},
        {"collover20_2d", collisionover20_2d, 7},
        {"collover13_3d", collisionover13_3d, 7},
        {"collover8_5d", collisionover8_5d, 7},
        {"collover5_8d", collisionover5_8d, 7},
        {"gap_inv512", gap_inv512, 13},
        {"hamming_dc6_values", hamming_dc6_values_test, 1},
        {"hamming_dc6_low1", hamming_dc6_low1_test, 1},
        {"linearcomp_high", linearcomp_high, 1},
        {"linearcomp_mid", linearcomp_mid, 1},
        {"linearcomp_low", linearcomp_low, 1},
        {NULL, NULL, 0}
    };

    const TestsBattery bat = {
        "brief", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }
}
