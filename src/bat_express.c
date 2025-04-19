/**
 * @file bat_express.c
 * @brief The `express` battery designed for memory constrained situations such
 * as 16-bit data segments (128 KiB of RAM per data and 64 KiB of RAM per code,
 * no arrays larger than 64 KiB) and absence of 64-bit arithmetics.
 * @details This battery is very fast but not very sensitive. Consumes only
 * 70-150 MiB of data on modern computers, runs in less than 1 second. Of course,
 * this implementation of battery is not designed for 16-bit platforms and ANSI C
 * compilers and made just for testing the concept.
 * 
 * If the same input data are used for different tests --- then amount of
 * required data may be reduced to 16 MiB that is comparable to DIEHARD test
 * suite.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_express.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/hwtests.h"
#include "smokerand/entropy.h"

/**
 * @brief SmokeRand `express` battery.
 */
void battery_express(const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype)
{
    static const BSpaceNDOptions
        bspace32_1d = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 1024, .get_lower = 1},
        bspace8_4d  = {.nbits_per_dim = 8,  .ndims = 4, .nsamples = 256,  .get_lower = 1},
        bspace4_8d  = {.nbits_per_dim = 4,  .ndims = 8, .nsamples = 128,  .get_lower = 1};
    static const BSpace4x8dDecimatedOptions bs_dec = {.step = 1 << 7};
    static const NBitWordsFreqOptions
        byte_freq   = {.bits_per_word = 8, .average_freq = 256, .nblocks = 256};

    // Linear complexity tests
    static const LinearCompOptions
        linearcomp_low  = {.nbits = 10000, .bitpos = LINEARCOMP_BITPOS_LOW},
        linearcomp_high = {.nbits = 10000, .bitpos = LINEARCOMP_BITPOS_HIGH};

    static const TestDescription tests[] = {
        {"byte_freq",       nbit_words_freq_test_wrap,      &byte_freq},
        {"bspace32_1d",     bspace_nd_test_wrap,            &bspace32_1d},
        {"bspace8_4d",      bspace_nd_test_wrap,            &bspace8_4d},
        {"bspace4_8d",      bspace_nd_test_wrap,            &bspace4_8d},
        {"bspace4_8d_dec",  bspace4_8d_decimated_test_wrap, &bs_dec},
        {"linearcomp_high", linearcomp_test_wrap,           &linearcomp_high},
        {"linearcomp_low",  linearcomp_test_wrap,           &linearcomp_low},
        {NULL, NULL, NULL}
    };

    const TestsBattery bat = {
        "express", tests
    };
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads, rtype);
    } else {
        TestsBattery_print_info(&bat);
    }
}
