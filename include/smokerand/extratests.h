/**
 * @file extratests.h
 * @brief Implementation of some statistical tests not included in the `brief`,
 * `default` and `full` batteries. These are 64-bit birthday paradox (not birthday
 * spacings!) test and 2D 16x16 Ising model tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_EXTRATESTS_H
#define __SMOKERAND_EXTRATESTS_H
#include "smokerand/core.h"

/**
 * @brief Settings for the birthday paradox test.
 */
typedef struct {
    size_t n; ///< Number of values
    unsigned int e; ///< Leave only values with zeros in lower (e - 1) bits
} BirthdayOptions;


TestResults birthday_test(GeneratorState *obj, const BirthdayOptions *opts);
void battery_birthday(GeneratorInfo *gen, const CallerAPI *intf);
void battery_ising(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads);
void battery_blockfreq(GeneratorInfo *gen, const CallerAPI *intf);



#endif
