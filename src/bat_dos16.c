/**
 * @file bat_dos16.h
 * @brief The `dos16` battery designed for memory constrained situations such
 * as 16-bit data segments (64 KiB of RAM per data and 64 KiB of RAM per code)
 * and absence of 64-bit arithmetics.
 * @details This battery is very fast but not very sensitive. Consumes 48 MiB
 * of data on modern computers, runs in less than 1 second. Of course, this
 * implementation of battery is not designed for 16-bit platforms and ANSI C
 * compilers and made just for testing the concept.
 * 
 * If the same input data are used for different tests --- then amount of
 * required data may be reduced to 16 MiB that is comparable to DIEHARD test
 * suite.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_dos16.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/hwtests.h"
#include "smokerand/entropy.h"

static TestResults bspace32_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 1024, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace4_8d_dec_test(GeneratorState *obj)
{
    return bspace4_8d_decimated_test(obj, 1<<7);
}


static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 10000, obj->gi->nbits - 1);
}

static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 10000, 0);
}

TestResults byte_freq_short_test(GeneratorState *obj)
{
    NBitWordsFreqOptions opts = {.bits_per_word = 8, .average_freq = 256,
        .nblocks = 256};
    return nbit_words_freq_test(obj, &opts);
}


void battery_dos16(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    static const TestDescription tests[] = {
        {"byte_freq", byte_freq_short_test, 2, ram_lo},
        {"bspace32_1d", bspace32_1d_test, 2, ram_hi},
        {"bspace4_8d_dec", bspace4_8d_dec_test, 3, ram_lo},
        {"linearcomp_high", linearcomp_high, 1, ram_lo},
        {"linearcomp_low", linearcomp_low, 1, ram_lo},
        {NULL, NULL, 0, 0}
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
