/**
 * @file bat_file.h
 * @brief A custom battery that is loaded from human-readable text file.
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_FILE_H
#define __SMOKERAND_BAT_FILE_H
#include "smokerand/core.h"

BatteryExitCode battery_file(const char *filename, const GeneratorInfo *gen,
    const CallerAPI *intf, const BatteryOptions *opts);
#endif
