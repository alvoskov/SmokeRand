/**
 * @file bat_express.h
 * @brief The `express` battery designed for memory constrained situations such
 * as 16-bit data segments (64 KiB of RAM per data and 64 KiB of RAM per code)
 * and absence of 64-bit arithmetics. Very fast but not very sensitive.
 * Consumes 2-4 GiB of data on modern computers, runs in less than 10 seconds.
 * @param Of course, this implementation of battery is not designed for 16-bit
 * platforms and ANSI C compilers and made just for testing the concept.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BAT_EXPRESS_H
#define __SMOKERAND_BAT_EXPRESS_H
#include "smokerand/core.h"
void battery_express(GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype);
#endif
