#include "smokerand/smokerand_core.h"
#include <stdio.h>


static inline void TestResults_print(const TestResults obj)
{
    printf("%s: p = %.3g; xemp = %g", obj.name, obj.p, obj.x);
    if (obj.p < 1e-10 || obj.p > 1 - 1e-10) {
        printf(" <<<<< FAIL\n");
    } else {
        printf("\n");
    }
}


void battery_default(GeneratorInfo *gen, CallerAPI *intf)
{
    printf("===== Starting 'default' battery =====\n");
    void *state = gen->create(intf);


    {
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .get_lower = 1};
    TestResults_print(bspace_nd_test(&opts, gen, state, intf));
    }

    {
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .get_lower = 1};
    TestResults_print(bspace_nd_test(&opts, gen, state, intf));
    }

    {
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .get_lower = 1};
    TestResults_print(bspace_nd_test(&opts, gen, state, intf));
    }

    {
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .get_lower = 1};
    TestResults_print(bspace_nd_test(&opts, gen, state, intf));
    }

    intf->free(state);
    
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        printf("Usage: smokerand battery generator_lib\n");
        printf(" battery: battery name; supported batteries:\n");
        printf("   - default\n");
        printf("  generator_lib: name of dynamic library with PRNG that export the functions:\n");
        printf("   - int gen_getinfo(GeneratorInfo *gi)\n");
        printf("\n");
        return 0;
    }
    CallerAPI intf = CallerAPI_init();
    char *battery_name = argv[1];
    char *generator_lib = argv[2];


    GeneratorModule mod = GeneratorModule_load(generator_lib);
    if (!mod.valid) {
        return 1;
    }
    

    printf("Battery name: %s\n", battery_name);

    if (!strcmp(battery_name, "default")) {
        battery_default(&mod.gen, &intf);
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        return 0;        
    }

    
    GeneratorModule_unload(&mod);
    return 0;
}
