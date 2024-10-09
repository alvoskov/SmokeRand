#include "smokerand/smokerand_core.h"
#include "smokerand/lineardep.h"
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


typedef struct {
    const char *name;
    TestResults (*run)(const GeneratorInfo *gen, void *state, const CallerAPI *intf);
} TestDescription;


TestDescription get_monobit_freq()
{
    TestDescription descr = {.name = "monobit_freq", .run = monobit_freq_test};
    return descr;
}

TestResults bspace32_1d_test(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .get_lower = 1};
    return bspace_nd_test(&opts, gen, state, intf);
}

TestResults bspace32_2d_test(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .get_lower = 1};
    return bspace_nd_test(&opts, gen, state, intf);
}

TestResults bspace21_3d_test(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .get_lower = 1};
    return bspace_nd_test(&opts, gen, state, intf);
}

TestResults bspace8_8d_test(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .get_lower = 1};
    return bspace_nd_test(&opts, gen, state, intf);
}


TestResults collisionover8_5d(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .get_lower = 1};
    return collisionover_test(&opts, gen, state, intf);
}

TestResults collisionover5_8d(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .get_lower = 1};
    return collisionover_test(&opts, gen, state, intf);
}

TestResults collisionover13_3d(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .get_lower = 1};
    return collisionover_test(&opts, gen, state, intf);
}

TestResults collisionover20_2d(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .get_lower = 1};
    return collisionover_test(&opts, gen, state, intf);
}


TestResults gap_inv256(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return gap_test(8, gen, state, intf);
}

TestResults gap_inv128(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return gap_test(7, gen, state, intf);
}



TestResults matrixrank_1024_low8(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return matrixrank_test(1024, 8, gen, state, intf);
}

TestResults matrixrank_4096_low8(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return matrixrank_test(4096, 8, gen, state, intf);
}


TestResults matrixrank_1024(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return matrixrank_test(1024, 64, gen, state, intf);
}

TestResults matrixrank_4096(const GeneratorInfo *gen, void *state, const CallerAPI *intf)
{
    return matrixrank_test(4096, 64, gen, state, intf);
}







void battery_default(GeneratorInfo *gen, CallerAPI *intf)
{
    printf("===== Starting 'default' battery =====\n");
    void *state = gen->create(intf);

    const TestDescription battery[] = {
        {"monobit_freq", monobit_freq_test},
        {"bspace32_1d", bspace32_1d_test},
        {"bspace32_2d", bspace32_2d_test},
        {"bspace21_3d", bspace21_3d_test},
        {"bspace8_8d", bspace8_8d_test},
        {"collover8_5d", collisionover8_5d},
        {"collover5_8d", collisionover5_8d},
        {"collover13_3d", collisionover13_3d},
        {"collover20_2d", collisionover20_2d},
        {"gap_inv128", gap_inv128},
        {"gap_inv256", gap_inv256},
        {"matrixrank_1024", matrixrank_1024},
        {"matrixrank_1024_low8", matrixrank_1024_low8},
        {"matrixrank_4096", matrixrank_4096},
        {"matrixrank_4096_low8", matrixrank_4096_low8},
        {NULL, NULL}
    };

    size_t ntests;
    for (ntests = 0; battery[ntests].run != NULL; ntests++) { }
    TestResults *results = calloc(ntests, sizeof(TestResults));

    for (size_t i = 0; i < ntests; i++) {
        results[i] = battery[i].run(gen, state, intf);
        results[i].name = battery[i].name;
    }

    intf->printf("  %20s %10s %10s\n", "Test name", "xemp", "p-value");
    for (size_t i = 0; i < ntests; i++) {
        intf->printf("  %20s %10.3g %10.3g %10s\n",
            results[i].name, results[i].x, results[i].p,
            interpret_pvalue(results[i].p));
    }
    free(results);

    intf->free(state);
    
}

void battery_self_test(GeneratorInfo *gen, const CallerAPI *intf)
{
    if (gen->self_test == 0) {
        intf->printf("Internal self-test not implemented\n");
        return;
    }
    intf->printf("Running internal self-test...\n");
    if (gen->self_test(intf)) {
        intf->printf("Internal self-test passed\n");
    } else {
        intf->printf("Internal self-test failed\n");
    }    
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        printf("Usage: smokerand battery generator_lib\n");
        printf(" battery: battery name; supported batteries:\n");
        printf("   - default\n");
        printf("   - selftest\n");
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
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(&mod.gen, &intf);
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        return 0;        
    }

    
    GeneratorModule_unload(&mod);
    return 0;
}
