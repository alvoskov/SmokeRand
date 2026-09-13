/**
 * @file lfsr_period_factors.h
 * @brief Contains factorizations for m - 1 for some m common for LFSR
 * generators.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_LFSR_PERIOD_FACTORS_H
#define __SMOKERAND_LFSR_PERIOD_FACTORS_H
#include "smokerand/lfsr_period.h"
const LargeInt *get_lfsr_exps(size_t n);
#endif // __SMOKERAND_LFSR_PERIOD_FACTORS_H
