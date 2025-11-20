/**
 * @file bat_brief.h
 * @brief The `brief` battery of tests that runs in about 1 minute.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_BRIEF_H
#define __SMOKERAND_BAT_BRIEF_H
#include "smokerand/core.h"
BatteryExitCode battery_brief(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts);
#endif
