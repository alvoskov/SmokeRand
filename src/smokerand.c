#include "smokerand/smokerand_core.h"
#include "smokerand/lineardep.h"
#include <stdio.h>
#include <time.h>



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
    TestResults (*run)(GeneratorState *obj);
} TestDescription;


TestDescription get_monobit_freq()
{
    TestDescription descr = {.name = "monobit_freq", .run = monobit_freq_test};
    return descr;
}

TestResults bspace32_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}



typedef struct {
    const GeneratorInfo *gen32;
    void *state;
} Bits64From32State;

static uint64_t get_bits64_from32(void *state)
{
    Bits64From32State *obj = state;
    uint64_t x = obj->gen32->get_bits(obj->state);
    uint64_t y = obj->gen32->get_bits(obj->state);
    return (x << 32) | y;
}

TestResults bspace64_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 64, .ndims = 1, .log2_len = 30, .get_lower = 1};
    if (obj->gi->nbits == 64) {
        return bspace_nd_test(obj, &opts);
    } else {
        GeneratorInfo gen32dup = *(obj->gi);
        gen32dup.get_bits = get_bits64_from32;
        Bits64From32State statedup = {obj->gi, obj->state};
        GeneratorState objdup = {.gi = &gen32dup, .state = &statedup, .intf = obj->intf};
        return bspace_nd_test(&objdup, &opts);
    }
}


TestResults bspace32_2d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .log2_len = 24, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

TestResults bspace21_3d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .log2_len = 24, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

TestResults bspace8_8d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .log2_len = 24, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}


TestResults collisionover8_5d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

TestResults collisionover5_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

TestResults collisionover13_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

TestResults collisionover20_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .get_lower = 1};
    return collisionover_test(obj, &opts);
}


TestResults gap_inv256(GeneratorState *obj)
{
    return gap_test(obj, 8);
}

TestResults gap_inv512(GeneratorState *obj)
{
    return gap_test(obj, 9);
}



TestResults matrixrank_1024_low8(GeneratorState *obj)
{
    return matrixrank_test(obj, 1024, 8);
}

TestResults matrixrank_4096_low8(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 8);
}


TestResults matrixrank_1024(GeneratorState *obj)
{
    return matrixrank_test(obj, 1024, 64);
}

TestResults matrixrank_4096(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 64);
}







void battery_default(GeneratorInfo *gen, CallerAPI *intf)
{
    printf("===== Starting 'default' battery =====\n");
    void *state = gen->create(intf);
    GeneratorState obj = {.gi = gen, .state = state, .intf = intf};

    const TestDescription battery[] = {
        {"monobit_freq", monobit_freq_test},
        {"byte_freq", byte_freq_test},
        {"bspace32_1d", bspace32_1d_test},
        {"bspace64_1d", bspace64_1d_test},
        {"bspace32_2d", bspace32_2d_test},
        {"bspace21_3d", bspace21_3d_test},
        {"bspace8_8d", bspace8_8d_test},
        {"collover8_5d", collisionover8_5d},
        {"collover5_8d", collisionover5_8d},
        {"collover13_3d", collisionover13_3d},
        {"collover20_2d", collisionover20_2d},
        {"gap_inv512", gap_inv512},
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

    time_t tic = time(NULL);
    for (size_t i = 0; i < ntests; i++) {
        results[i] = battery[i].run(&obj);
        results[i].name = battery[i].name;
    }
    time_t toc = time(NULL);

    intf->printf("  %20s %10s %10s %10s\n", "Test name", "xemp", "p", "1 - p");
    for (size_t i = 0; i < ntests; i++) {
        intf->printf("  %20s %10.3g %10.3g %10.3g %10s\n",
            results[i].name, results[i].x, results[i].p,
            results[i].alpha,
            interpret_pvalue(results[i].p));
    }
    unsigned int nseconds_total = toc - tic;
    int s = nseconds_total % 60;
    int m = (nseconds_total / 60) % 60;
    int h = (nseconds_total / 3600);
    intf->printf("Elapsed time: %.2d:%.2d:%.2d\n", h, m, s);
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
