#include "smokerand/smokerand_core.h"

int main()
{
    CallerAPI intf = CallerAPI_init();
    GeneratorModule mod = GeneratorModule_load("generators/libspeck128_avx_shared.dll");
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }




    GeneratorState_free(&obj, &intf);
    GeneratorModule_unload(&mod);
    CallerAPI_free();
    return 0;
}
