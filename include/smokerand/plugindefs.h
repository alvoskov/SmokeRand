/**
 * @file plugindefs.h
 * @brief Header file that should be included in all custom plugins,
 * see `apps\bat_example.c` as an example. It protects from incorrect
 * definition of the exported function.
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_PLUGINDEFS_H
#define __SMOKERAND_PLUGINDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "smokerand/apidefs.h"
#include "smokerand/core.h"

BatteryExitCode EXPORT battery_func(const GeneratorInfo *gen,
    const CallerAPI *intf, const BatteryOptions *opts);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SMOKERAND_PLUGINDEFS_H
