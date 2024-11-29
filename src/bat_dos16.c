/**
 * @file bat_dos16.h
 * @brief The `dos16` battery designed for memory constrained situations such
 * as 16-bit data segments (64 KiB of RAM per data and 64 KiB of RAM per code)
 * and absence of 64-bit arithmetics. Very fast but not very sensitive.
 * Consumes 2-4 GiB of data on modern computers, runs in less than 10 seconds.
 * @param Of course, this implementation of battery is not designed for 16-bit
 * platforms and ANSI C compilers and made just for testing the concept.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_dos16.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"

static TestResults bspace32_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 4096, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_8d_dec_test(GeneratorState *obj)
{
    return bspace4_8d_decimated_test(obj, 1<<12);
}


static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 50000, obj->gi->nbits - 1);
}

static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 50000, 0);
}

static TestResults gap_ndnd(GeneratorState *obj)
{
    GapNDOptions opts = {.ngaps = 1000000};
    return gap_nd_test(obj, &opts );
}


void battery_dos16(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    static const TestDescription tests[] = {
//        {"mod3", mod3_test, 2},
        {"gap_nd", gap_ndnd, 2},
        {"byte_freq", byte_freq_test, 2},
        {"bspace32_1d", bspace32_1d_test, 2},
        {"bspace4_8d_dec", bspace4_8d_dec_test, 3},
        {"linearcomp_high", linearcomp_high, 1},
        {"linearcomp_low", linearcomp_low, 1},
        {NULL, NULL, 0}
    };

    const TestsBattery bat = {
        "dos16", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }
}
