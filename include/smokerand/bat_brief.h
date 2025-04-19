/**
 * @file bat_brief.h
 * @brief The `brief` battery of tests that runs in about 1 minute.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_BRIEF_H
#define __SMOKERAND_BAT_BRIEF_H
#include "smokerand/core.h"
void battery_brief(const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype);
#endif
