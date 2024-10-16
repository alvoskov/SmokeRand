/**
 * @file lineardep.h
 * @brief Implementation of linear complexity and matrix rank tests.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_LINEARDEP_H
#define __SMOKERAND_LINEARDEP_H
#include "smokerand/core.h"
TestResults linearcomp_test(GeneratorState *obj, size_t nbits, unsigned int bitpos);
TestResults matrixrank_test(GeneratorState *obj, size_t n, unsigned int max_nbits);
#endif

