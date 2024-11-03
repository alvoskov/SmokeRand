/**
 * @file fileio.h
 * @brief Implementation of PRNG based on reading binary data from stdin.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_FILEIO_H
#define __SMOKERAND_FILEIO_H
#include "smokerand/core.h"

typedef enum {
    stdin_collector_32bit,
    stdin_collector_64bit
} StdinCollectorType;

GeneratorInfo StdinCollector_get_info(StdinCollectorType type);
void StdinCollector_print_report();
#endif
