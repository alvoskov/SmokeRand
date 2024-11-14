/**
 * @file bat_antilcg.c
 * @brief The `antilcg` designed against linear congruental generators.
 * @details This battery is based on different variations of birthday spacings
 * test, detects 64-bit LCG with prime modulus (just as `brief` battery) but
 * also can detect 128-bit LCG with modulus \f$ 2^{128} \f$ and truncated lower
 * 96 bits, i.e. 32-bit output. The last test runs too long (about 20 minutes)
 * to be included into `full` battery.
 *
 * It also detects additive/subtractive lagged Fibonacci generators, subtract
 * with borrow generators but is not sensitive to LFSR (linear feedback shift
 * register) generators.
 */
#include "smokerand/bat_antilcg.h"
#include "smokerand/coretests.h"
#include "smokerand/entropy.h"

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

static TestResults bspace21_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 100, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 100, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 50, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 50, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_dec(GeneratorState *obj)
{
    return bspace8_8d_decimated_test(obj, 8192);
}

void battery_antilcg(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    static const TestDescription tests[] = {

        {"bspace64_1d", bspace64_1d, 90},
        {"bspace32_1d", bspace32_1d, 2},
        {"bspace32_1d_high", bspace32_1d_high, 2},
        {"bspace21_3d", bspace21_3d, 77},
        {"bspace21_3d_high", bspace21_3d_high, 77},
        {"bspace8_8d", bspace8_8d, 120},
        {"bspace8_8d_high", bspace8_8d_high, 120},
        {"bspace8_8d_dec", bspace8_8d_dec, 75},
        {NULL, NULL, 0}
    };

    const TestsBattery bat = {
        "antilcg", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }
}
