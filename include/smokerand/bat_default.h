/**
 * @file bat_default.h
 * @brief The `default` battery of tests that runs in about 5 minutes.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_DEFAULT_H
#define __SMOKERAND_BAT_DEFAULT_H
#include "smokerand/core.h"
BatteryExitCode battery_default(const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype);
#endif
