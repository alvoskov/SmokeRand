#include "smokerand/bat_default.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"

static TestResults bspace32_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 1, .nsamples = 8192, .get_lower = 1};
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


static TestResults bspace64_1d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 64, .ndims = 1, .nsamples = 50, .get_lower = 1};
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


static TestResults bspace32_2d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace32_2d_test_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 32, .ndims = 2, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace21_3d_test_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 21, .ndims = 3, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace16_4d_test_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 16, .ndims = 4, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults bspace8_8d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace8_8d_test_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 8, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d_test(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 10, .get_lower = 1};
    return bspace_nd_test(obj, &opts);
}

static TestResults bspace4_16d_test_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 4, .ndims = 16, .nsamples = 10, .get_lower = 0};
    return bspace_nd_test(obj, &opts);
}


static TestResults collisionover8_5d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover8_5d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 8, .ndims = 5, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover5_8d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 5, .ndims = 8, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover13_3d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 13, .ndims = 3, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .get_lower = 1};
    return collisionover_test(obj, &opts);
}

static TestResults collisionover20_2d_high(GeneratorState *obj)
{
    BSpaceNDOptions opts = {.nbits_per_dim = 20, .ndims = 2, .get_lower = 0};
    return collisionover_test(obj, &opts);
}

static TestResults gap_inv512(GeneratorState *obj)
{
    return gap_test(obj, 9);
}


static TestResults matrixrank_4096(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 64);
}

static TestResults matrixrank_4096_low8(GeneratorState *obj)
{
    return matrixrank_test(obj, 4096, 8);
}

static TestResults linearcomp_high(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, obj->gi->nbits - 1);
}

static TestResults linearcomp_mid(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, obj->gi->nbits / 2 - 1);
}


static TestResults linearcomp_low(GeneratorState *obj)
{
    return linearcomp_test(obj, 100000, 0);
}

void battery_default(GeneratorInfo *gen, CallerAPI *intf, unsigned int nthreads)
{
    const TestDescription tests[] = {
        {"monobit_freq", monobit_freq_test},
        {"byte_freq", byte_freq_test},
        {"bspace64_1d", bspace64_1d_test},
        {"bspace32_1d", bspace32_1d_test},
        {"bspace32_2d", bspace32_2d_test},
        {"bspace32_2d_high", bspace32_2d_test_high},
        {"bspace21_3d", bspace21_3d_test},
        {"bspace21_3d_high", bspace21_3d_test_high},
        {"bspace16_4d", bspace16_4d_test},
        {"bspace16_4d_high", bspace16_4d_test_high},
        {"bspace8_8d", bspace8_8d_test},
        {"bspace8_8d_high", bspace8_8d_test_high},
        {"bspace4_16d", bspace4_16d_test},
        {"bspace4_16d_high", bspace4_16d_test_high},
        {"collover20_2d", collisionover20_2d},
        {"collover20_2d_high", collisionover20_2d_high},
        {"collover13_3d", collisionover13_3d},
        {"collover13_3d_high", collisionover13_3d_high},
        {"collover8_5d", collisionover8_5d},
        {"collover8_5d_high", collisionover8_5d_high},
        {"collover5_8d", collisionover5_8d},
        {"collover5_8d_high", collisionover5_8d_high},
        {"gap_inv512", gap_inv512},
        {"linearcomp_high", linearcomp_high},
        {"linearcomp_mid", linearcomp_mid},
        {"linearcomp_low", linearcomp_low},
        {"matrixrank_4096", matrixrank_4096},
        {"matrixrank_4096_low8", matrixrank_4096_low8},
        {NULL, NULL}
    };

    const TestsBattery bat = {
        "default", tests
    };
    TestsBattery_run(&bat, gen, intf, nthreads);
}
