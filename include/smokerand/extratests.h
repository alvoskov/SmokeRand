/**
 * @file extratests.h
 * @brief Implementation of some statistical tests not included in the `express`,
 * `brief`, `default` and `full` batteries. These are 64-bit collision test
 * (the former "birthday paradox test"), 2D 16x16 Ising model tests and adaptive
 * frequency test for 8-bit and 16-bit blocks.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_EXTRATESTS_H
#define __SMOKERAND_EXTRATESTS_H
#include "smokerand/core.h"

/**
 * @brief Settings for the 64-bit collision test with decimtion
 * (the former "birthday paradox test").
 */
typedef struct {
    unsigned long long n; ///< Number of values
    unsigned int e; ///< Leave only values with zeros in lower (e - 1) bits
    unsigned int nbits_per_value; ///< 32 or 64
    uint64_t mvalue; ///< Required value in the lower (e - 1) bits
    int is_dynamic_mvalue;
    unsigned int niters_max; ///< Maximal number of iterations
} CollOver64DecimatedOptions;


/**
 * @brief Algorithms for the PRNG test based on Ising model.
 */
typedef enum {
    ISING_WOLFF, ///< Wolff algorithm
    ISING_METROPOLIS ///< Metropolis algorithm
} IsingAlgorithm;


/**
 * @brief Options for the PRNG test based on 2D Ising model.
 */
typedef struct {
    IsingAlgorithm algorithm; ///< Used algorithm (Metropolis, Wolff etc.)
    unsigned long sample_len; ///< Number of calls per sample
    unsigned int nsamples; ///< Number of samples for computation of E and C
} Ising2DOptions;

/**
 * @brief Options for the PRNG test based on the volumes of n-dimensional
 * unit spheres (Monte-Carlo computation of pi)
 */
typedef struct {
    unsigned int ndims;
    unsigned long long npoints;
} UnitSphereOptions;


TestResults ising2d_test(GeneratorState *obj, const Ising2DOptions *opts);
TestResults unit_sphere_volume_test(GeneratorState *gs, const UnitSphereOptions *opts);

TestResults ising2d_test_wrap(GeneratorState *obj, const void *udata);
TestResults unit_sphere_volume_test_wrap(GeneratorState *gs, const void *udata);


BatteryExitCode battery_collover64_decimated(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *bat_opts);
BatteryExitCode battery_ising(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts);
BatteryExitCode battery_blockfreq(const GeneratorInfo *gen, const CallerAPI *intf);
BatteryExitCode battery_unit_sphere_volume(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts);


#endif // __SMOKERAND_EXTRATESTS_H
