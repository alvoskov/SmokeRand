/**
 * @file bat_special.h
 * @brief Contains implementations of special pseudobatteries for measuring
 * PRNG speed, running its internal self-tests etc.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_SPECIAL_H
#define __SMOKERAND_BAT_SPECIAL_H
#include "smokerand/core.h"

/**
 * @brief Keeps PRNG speed measurements results.
 */
typedef struct {
    double ns_per_call; ///< Nanoseconds per call
    double ticks_per_call; ///< Processor ticks per call
    double cpb; ///< cpb (cycles/CPU ticks per byte)
    double bytes_per_sec; ///< Bytes per second
    double cpu_freq_meas; ///< Measured CPU frequency, MHz
} SpeedResults;

/**
 * @brief Keeps PRNG speed measurements results for raw speed,
 * baseline speed and corrected speed.
 */
typedef struct {
    SpeedResults full; ///< Before correction.
    SpeedResults baseline; ///< For baseline ("dummy" PRNG)
    SpeedResults corr; ///< Corrected results (with baseline subtraction)
} SpeedResultsAll;


typedef struct {
    SpeedResultsAll uint;
    SpeedResultsAll sum;
    SpeedResultsAll mean;
} SpeedBatteryResults;

/**
 * @brief Speed measurement mode.
 */
typedef enum {
    SPEED_UINT, ///< Single value (emulates call of PRNG function)
    SPEED_SUM   ///< Sum of values (emulates usage of inline PRNG)
} SpeedMeasurementMode;

/**
 * @brief Converts bytes to GiB (gibibytes)
 */
static inline double nbytes_to_gib(double nbytes)
{
    static const double b_to_gib = 1.0 / (double) (1UL << 30);
    return nbytes * b_to_gib;
}

/**
 * @brief Converts bytes to MiB (mebibytes)
 */
static inline double nbytes_to_mib(double nbytes)
{
    static const double b_to_gib = 1.0 / (double) (1UL << 20);
    return nbytes * b_to_gib;
}


SpeedBatteryResults SpeedBatteryResults_get(const GeneratorInfo *gen, const CallerAPI *intf);


BatteryExitCode battery_speed(const GeneratorInfo *gen, const CallerAPI *intf);
BatteryExitCode battery_self_test(const GeneratorInfo *gen, const CallerAPI *intf);

#endif
