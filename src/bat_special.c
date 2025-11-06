#include "smokerand/entropy.h"
#include "smokerand/bat_special.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SUM_BLOCK_SIZE 32768

/**
 * @brief Keeps PRNG speed measurements results.
 */
typedef struct
{
    double ns_per_call; ///< Nanoseconds per call
    double ticks_per_call; ///< Processor ticks per call
    double cpb; ///< cpb (cycles/CPU ticks per byte)
} SpeedResults;

/**
 * @brief Speed measurement mode.
 */
typedef enum {
    SPEED_UINT, ///< Single value (emulates call of PRNG function)
    SPEED_SUM   ///< Sum of values (emulates usage of inline PRNG)
} SpeedMeasurementMode;


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
    for (size_t niter = 2; ns_total < 0.5e9; niter <<= 1) {
        clock_t tic = clock();
        uint64_t tic_proc = cpuclock();
        if (mode == SPEED_UINT) {
            uint64_t sum = 0;
            for (size_t i = 0; i < niter; i++) {
                sum += obj.gi->get_bits(obj.state);
            }
            (void) sum;
        } else {
            uint64_t sum = 0;
            for (size_t i = 0; i < niter; i++) {
                sum += obj.gi->get_sum(obj.state, SUM_BLOCK_SIZE);
            }
            (void) sum;
        }
        uint64_t toc_proc = cpuclock();
        clock_t toc = clock();
        ns_total = 1.0e9 * (toc - tic) / CLOCKS_PER_SEC;
        results.ns_per_call = ns_total / (double) niter;
        results.ticks_per_call = (double) (toc_proc - tic_proc) / (double) niter;
        // Convert to cpb
        size_t block_size = (mode == SPEED_UINT) ? 1 : SUM_BLOCK_SIZE;
        size_t nbytes = block_size * gen->nbits / 8;
        results.cpb = results.ticks_per_call / (double) nbytes;
    }
    GeneratorState_destruct(&obj, intf);
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



SpeedResults battery_speed_test(const GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorInfo dummy_gen = {.name = "dummy", .description = "DUMMY",
        .create = dummy_create, .free = dummy_free,
        .get_bits = dummy_get_bits, .get_sum = dummy_get_sum,
        .self_test = NULL, .parent = NULL};
    dummy_gen.nbits = gen->nbits;
    SpeedResults speed_full = measure_speed(gen, intf, mode);
    SpeedResults speed_dummy = measure_speed(&dummy_gen, intf, mode);
    SpeedResults speed_corr;
    size_t block_size = (mode == SPEED_UINT) ? 1 : SUM_BLOCK_SIZE;
    size_t nbytes = block_size * gen->nbits / 8;
    speed_corr.ns_per_call = speed_full.ns_per_call - speed_dummy.ns_per_call;
    speed_corr.ticks_per_call = speed_full.ticks_per_call - speed_dummy.ticks_per_call;
    speed_corr.cpb = speed_full.cpb - speed_dummy.cpb;
    if (speed_corr.cpb <= 0.0) {
        speed_corr.cpb = NAN;
    }

    //double cpb_corr = ticks_per_call_corr / nbytes;
    double gb_per_sec = (double) nbytes / (1.0e-9 * speed_corr.ns_per_call) / pow(2.0, 30.0);
    // Print report
    printf("Nanoseconds per call:\n");
    printf("  Raw result:                 %g\n", speed_full.ns_per_call);
    printf("  For empty 'dummy' PRNG:     %g\n", speed_dummy.ns_per_call);
    printf("  Corrected result:           %g\n", speed_corr.ns_per_call);
    printf("  Corrected result (GiB/sec): %g\n", gb_per_sec);
    printf("CPU frequency:                %.1f MHz\n",
        speed_full.ticks_per_call / speed_full.ns_per_call * 1000.0);
    printf("CPU ticks per call:\n");
    printf("  Raw result:                 %g\n", speed_full.ticks_per_call);
    printf("  Raw result (cpB):           %g\n", speed_full.cpb);
    printf("  For empty 'dummy' PRNG:     %g\n", speed_dummy.ticks_per_call);
    printf("  Corrected result:           %g\n", speed_corr.ticks_per_call);
    printf("  Corrected result (cpB):     %g\n\n", speed_corr.cpb);
    // Corrected results
    return speed_corr;
}

BatteryExitCode battery_speed(const GeneratorInfo *gen, const CallerAPI *intf)
{
    printf("===== Generator speed measurements =====\n");
    printf("----- Speed test for uint generation -----\n");
    SpeedResults res_uint = battery_speed_test(gen, intf, SPEED_UINT);
    printf("----- Speed test for uint sum generation -----\n");
    if (gen->get_sum == NULL) {
        printf("  Not implemented\n");
    } else {
        SpeedResults res_sum = battery_speed_test(gen, intf, SPEED_SUM);
        double cpb_mean;
        if (res_uint.cpb < 0.25 && res_sum.cpb > 0.0 &&
            res_sum.cpb > res_uint.cpb) {
            cpb_mean = res_sum.cpb;
        } else {
            cpb_mean = (res_uint.cpb + res_sum.cpb) / 2.0;
        }
        printf("Average results:\n");
        printf("  Corrected result (cpB):     %g\n\n", cpb_mean);
    }
    return BATTERY_PASSED;
}

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
