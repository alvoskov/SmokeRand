/**
 * @file lfsr_period.h
 * @brief Simple tools for proving the LFSR period using the theoretical
 * (algebraic) methods. Allow to check small xorshift-style generators with
 * states up to 256 bits.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_LFSR_PERIOD_H
#define __SMOKERAND_LFSR_PERIOD_H
#include "smokerand/core.h"

typedef struct {
    int check_validity;
} LfsrPeriodOptions;


typedef enum {
    LFSR_PERIOD_MAX     = 0,
    LFSR_PERIOD_NOT_MAX = 1,
    LFSR_PERIOD_ERROR   = 2
} LfsrPeriodResult;

typedef struct {
    GeneratorState state;
    size_t nbytes; ///< State size in bytes
}  GeneratorStateExt;

GeneratorStateExt
GeneratorStateExt_create(const GeneratorInfo *gen, const CallerAPI *intf);
int GeneratorStateExt_is_valid(GeneratorStateExt *obj, const CallerAPI *intf);
void GeneratorStateExt_destruct(GeneratorStateExt *obj);

void LfsrPeriodResult_print(const CallerAPI *intf, LfsrPeriodResult res);
LfsrPeriodResult lfsr_period_test(GeneratorStateExt *ext, const CallerAPI *intf,
    const LfsrPeriodOptions *opts);
BatteryExitCode battery_lfsr_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts);

#endif // __SMOKERAND_LFSR_PERIOD_H
