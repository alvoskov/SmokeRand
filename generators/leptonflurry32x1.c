/**
 * @file leptonflurry32x1.c
 * @brief Leptonflurry32x1 is a counter based PRNG developed by William Stafford
 * Parsons. It uses only 32-bit integers and is fairly slow at CPU (around 2-3 cpb).
 * 
 * References:
 *
 * 1. https://github.com/eightomic/leptonflurry/blob/master/leptonflurry.c
 * 2. https://eightomic.com/
 *
 * @copyright LeptonFlurry PRNG family was developed by
 * William Stafford Parsons.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LeptonFlurry32x1 PRNG state.
 */
typedef struct {
    uint32_t ctr[2];
    uint32_t key[2];
} LeptonFlurry32x1;


static inline uint64_t get_bits_v1_raw(LeptonFlurry32x1 *obj)
{
    uint32_t a = obj->key[0] + 11111U, b = obj->key[1] + 1111111111U;

    a += a << 15;
    a += obj->ctr[0] ^ b;
    a += a << 23;
    a ^= a >> 7;

    b += a + (a << 13);
    b += obj->ctr[1];
    b += b << 13;
    b ^= b >> 5;

    if (++obj->ctr[0] == 0) obj->ctr[1]++;

    a += b + (b << 19); b += a + (a << 23);
    a ^= b ^ (b >> 17); b += a + (a << 7);
    a ^= b ^ (b >> 13); b ^= a ^ (a >> 15);
    a += b + (b << 9);  b += a + (a << 17);
    return (a ^ (a >> 7)) + (b ^ (b >> 11));
}

MAKE_GET_BITS_WRAPPERS(v1)


static inline uint32_t mix32(uint32_t x, int sh1, int sh2)
{
    return (x + (x << sh1)) ^ (x >> sh2);
}

static inline uint64_t get_bits_v2_raw(LeptonFlurry32x1 *obj)
{
    uint32_t a = obj->ctr[0] + obj->key[0] + 11111111U;
    uint32_t b = obj->ctr[1] + obj->key[1];

    a += (a << 9) + obj->ctr[1] + b;
    a += a << 17;
    a ^= a >> 13;
    b += obj->ctr[0] + a;

    if (++obj->ctr[0] == 0) obj->ctr[1]++;

    a += mix32(b, 7, 11);
    b += mix32(a, 17, 13);
    a += mix32(b, 7, 7);
    b += mix32(a, 13, 11);
    return a + mix32(b, 9, 5);
}

MAKE_GET_BITS_WRAPPERS(v2)




static void *create(const CallerAPI *intf)
{
    LeptonFlurry32x1 *obj = intf->malloc(sizeof(LeptonFlurry32x1));
    seed64_to_2x32(intf, &obj->key[0], &obj->key[1]);
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    return obj;
}



static const GeneratorParamVariant gen_list[] = {
    {"",          "leptonflurry32x1:v2", 32, default_create, get_bits_v2, get_sum_v2},
    {"v2",        "leptonflurry32x1:v2", 32, default_create, get_bits_v2, get_sum_v2},
    {"v1",        "leptonflurry32x1:v1", 32, default_create, get_bits_v1, get_sum_v1},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The leptonflurry32x1 is a counter-based PRNG.\n"
"The next param values are supported:\n"
"  v2 - version 2 (default version)\n"
"  v1 - version 1\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
