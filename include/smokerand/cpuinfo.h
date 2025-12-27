/**
 * @file cpuinfo.h
 * @brief Functions that give access to CPU tick counters, frequency information
 * and other characteristics.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_CPUINFO_H
#define __SMOKERAND_CPUINFO_H
#include <stdint.h>
uint64_t cpuclock();
uint64_t call_rdseed();
double get_cpu_freq(void);
#endif // __SMOKERAND_CPUINFO_H
