/**
 * @file bat_full.h
 * @brief The `antilcg` designed against linear congruental generators.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_ANTILCG_H
#define __SMOKERAND_BAT_ANTILCG_H
#include "smokerand/core.h"
void battery_antilcg(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads);
#endif
