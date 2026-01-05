/**
 * @file bat_special.c
 * @brief Contains implementations of special pseudobatteries for measuring
 * PRNG speed, running its internal self-tests etc.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cpuinfo.h"
#include "smokerand/entropy.h"
#include "smokerand/bat_special.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SUM_BLOCK_SIZE 32768

/**
 * @brief PRNG speed measurement for uint output.
 * @details Two modes are supported: measurement of one function call
 * and measurement of sum computation by PRNG inside the cycle.
 */
static SpeedResults measure_speed(const GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorState obj = GeneratorState_create(gen, intf);
    SpeedResults results;
    double ns_total = 0.0;
    for (unsigned long long niter = 2; ns_total < 0.5e9; niter <<= 1) {
        clock_t tic = clock();
        uint64_t tic_proc = cpuclock();
        if (mode == SPEED_UINT) {
            uint64_t sum = 0;
            for (unsigned long long i = 0; i < niter; i++) {
                sum += obj.gi->get_bits(obj.state);
            }
            (void) sum;
        } else {
            uint64_t sum = 0;
            for (unsigned long long i = 0; i < niter; i++) {
                sum += obj.gi->get_sum(obj.state, SUM_BLOCK_SIZE);
            }
            (void) sum;
        }
        uint64_t toc_proc = cpuclock();
        clock_t toc = clock();
        ns_total = 1.0e9 * ((double) (toc - tic) / (double) CLOCKS_PER_SEC);
        results.ns_per_call = ns_total / (double) niter;
        results.ticks_per_call = (double) (toc_proc - tic_proc) / (double) niter;
        // Convert to cpb
        size_t block_size = (mode == SPEED_UINT) ? 1 : SUM_BLOCK_SIZE;
        size_t nbytes = block_size * gen->nbits / 8;
        results.cpb = results.ticks_per_call / (double) nbytes;
        // Convert to bytes per second
        results.bytes_per_sec = (double) nbytes / (1.0e-9 * results.ns_per_call);
        // Estimate CPU frequency (in MHz)
        results.cpu_freq_meas = results.ticks_per_call / results.ns_per_call * 1000.0;
    }
    GeneratorState_destruct(&obj);
    return results;
}

static void *dummy_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    return intf->malloc(1);
}

static void dummy_free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    intf->free(state);
}

static uint64_t dummy_get_bits(void *state)
{
    (void) state;
    return 0;
}

/**
 * @brief Sums some predefined elements in the cycle to measure
 * baseline time for PRNG performance estimation. It intentionally
 * uses a table with numbers to override optimization of compilers
 * (exclusion of cycle, even replacement of cycle for arithmetic
 * progression to formula etc.) Inspired by DOOM PRNG.
 */
static uint64_t dummy_get_sum(void *state, size_t len)
{
    uint64_t data[] = {9338, 34516, 60623, 45281,
        9064,   60090,  62764,  5557,
        44347,  35277,  25712,  20552,
        50645,  61072,  26719,  21307};
    uint64_t sum = 0;
    (void) state;
    for (size_t i = 0; i < len; i++) {
        sum += data[i & 0xF];
    }
    return sum;
}


SpeedResults
SpeedResults_subtract_baseline(const SpeedResults *full, const SpeedResults *baseline)
{
    SpeedResults corr;
    corr.ns_per_call    = full->ns_per_call    - baseline->ns_per_call;
    corr.ticks_per_call = full->ticks_per_call - baseline->ticks_per_call;
    corr.cpb            = full->cpb            - baseline->cpb;
    corr.bytes_per_sec  = 1.0 / (1.0/full->bytes_per_sec - 1.0/baseline->bytes_per_sec);
    corr.cpu_freq_meas  = 0.5 * (full->cpu_freq_meas + baseline->cpu_freq_meas);
    if (corr.ns_per_call    <= 0.0) corr.ns_per_call = NAN;
    if (corr.ticks_per_call <= 0.0) corr.ticks_per_call = NAN;
    if (corr.cpb            <= 0.0) corr.cpb = NAN;
    if (corr.bytes_per_sec  <= 0.0) corr.bytes_per_sec = NAN;
    return corr;
}


SpeedResults SpeedResults_calc_mean(const SpeedResults *a, const SpeedResults *b)
{
    SpeedResults res;
    res.ns_per_call    = 0.5 * (a->ns_per_call    + b->ns_per_call);
    res.ticks_per_call = 0.5 * (a->ticks_per_call + b->ticks_per_call);
    res.cpb            = 0.5 * (a->cpb            + b->cpb);
    res.bytes_per_sec  = 2.0 / (1.0/a->bytes_per_sec + 1.0/b->bytes_per_sec);
    res.cpu_freq_meas  = 0.5 * (a->cpu_freq_meas  + b->cpu_freq_meas);
    return res;
}

void SpeedResultsAll_print(const SpeedResultsAll *speed)
{
    printf("Nanoseconds per call:\n");
    printf("  Raw result:                 %g\n", speed->full.ns_per_call);
    printf("  For empty 'dummy' PRNG:     %g\n", speed->baseline.ns_per_call);
    printf("  Corrected result:           %g\n", speed->corr.ns_per_call);
    printf("  Corrected result (GiB/sec): %g\n", nbytes_to_gib(speed->corr.bytes_per_sec));
    printf("CPU frequency:                %.1f MHz\n", speed->corr.cpu_freq_meas);
    printf("CPU ticks per call:\n");
    printf("  Raw result:                 %g\n", speed->full.ticks_per_call);
    printf("  Raw result (cpB):           %g\n", speed->full.cpb);
    printf("  For empty 'dummy' PRNG:     %g\n", speed->baseline.ticks_per_call);
    printf("  Corrected result:           %g\n", speed->corr.ticks_per_call);
    printf("  Corrected result (cpB):     %g\n\n", speed->corr.cpb);
}


SpeedResultsAll battery_speed_test(const GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorInfo dummy_gen = {.name = "dummy", .description = "DUMMY",
        .create = dummy_create, .free = dummy_free,
        .get_bits = dummy_get_bits, .get_sum = dummy_get_sum,
        .self_test = NULL, .parent = NULL};
    dummy_gen.nbits = gen->nbits;
    SpeedResultsAll speed;
    speed.full     = measure_speed(gen, intf, mode);
    speed.baseline = measure_speed(&dummy_gen, intf, mode);
    speed.corr     = SpeedResults_subtract_baseline(&speed.full, &speed.baseline);
    // Print report
    SpeedResultsAll_print(&speed);
    // Corrected results
    return speed;
}


SpeedBatteryResults SpeedBatteryResults_get(const GeneratorInfo *gen, const CallerAPI *intf)
{
    SpeedBatteryResults res;
    printf("===== Generator speed measurements =====\n");
    printf("----- Speed test for uint generation -----\n");
    res.uint = battery_speed_test(gen, intf, SPEED_UINT);
    printf("----- Speed test for uint sum generation -----\n");
    if (gen->get_sum == NULL) {
        res.mean = res.uint;
        printf("  Not implemented\n");
    } else {
        res.sum = battery_speed_test(gen, intf, SPEED_SUM);
        if (res.uint.corr.cpb < 0.25 && res.sum.corr.cpb > 0.0 &&
            res.sum.corr.cpb > res.uint.corr.cpb) {
            res.mean = res.sum;
        } else {
            res.mean.full     = SpeedResults_calc_mean(&res.uint.full, &res.sum.full);
            res.mean.baseline = SpeedResults_calc_mean(&res.uint.baseline, &res.sum.baseline);
            res.mean.corr     = SpeedResults_calc_mean(&res.uint.corr, &res.sum.corr);
        }
        printf("Average results:\n");
        printf("  Corrected result (GiB/s):   %g\n",   nbytes_to_gib(res.mean.corr.bytes_per_sec));
        printf("  Corrected result (cpB):     %g\n\n", res.mean.corr.cpb);
    }
    return res;
}

/**
 * @brief Measures pseudorandom number generator performance/speed
 * in different modes
 */
BatteryExitCode battery_speed(const GeneratorInfo *gen, const CallerAPI *intf)
{
    (void) SpeedBatteryResults_get(gen, intf);
    return BATTERY_PASSED;
}

/**
 * @brief Runs an internal self-test for the generator.
 */
BatteryExitCode battery_self_test(const GeneratorInfo *gen, const CallerAPI *intf)
{
    if (gen->self_test == 0) {
        intf->printf("Internal self-test not implemented\n");
        return BATTERY_ERROR;
    }
    intf->printf("Running internal self-test...\n");
    if (gen->self_test(intf)) {
        intf->printf("Internal self-test passed\n");
        return BATTERY_PASSED;
    } else {
        intf->printf("Internal self-test failed\n");
        return BATTERY_FAILED;
    }
}
