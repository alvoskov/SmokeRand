#ifndef __SMOKERAND_BAT_SPECIAL_H
#define __SMOKERAND_BAT_SPECIAL_H
#include "smokerand/core.h"

void battery_speed(const GeneratorInfo *gen, const CallerAPI *intf);
void battery_self_test(const GeneratorInfo *gen, const CallerAPI *intf);

#endif
