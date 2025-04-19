/**
 * @file bat_full.h
 * @brief The `full` battery of tests that runs in about 1 hour.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_FULL_H
#define __SMOKERAND_BAT_FULL_H
#include "smokerand/core.h"
void battery_full(const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype);
#endif
