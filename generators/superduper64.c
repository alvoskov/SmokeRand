/**
 * @file superduper64.c
 * @brief An implementation of 64-bit combined "Super Duper" PRNG
 * by G. Marsaglia. It is a combination of 64-bit LCG and 64-bit xorshift.
 * 
 * @details Supports the 64-bit output (default or `--param=u64`) and
 * 32-bit output (`--param=u32`, upper 32 bits are returned). Only the
 * `u32` version passes all SmokeRand batteries.
 *
 * References:
 *
 * https://groups.google.com/g/comp.sys.sun.admin/c/GWdUThc_JUg/m/_REyWTjwP7EJ
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief SuperDuper64 PRNG state
 */
typedef struct {
    uint64_t lcg;
    uint64_t xs;
} SuperDuper64State;

static inline uint64_t superduper64_get_bits(SuperDuper64State *obj)
{
    obj->lcg = 6906969069U * obj->lcg + 1234567U;
    obj->xs ^= (obj->xs << 13);
    obj->xs ^= (obj->xs >> 17);
    obj->xs ^= (obj->xs << 43);
    return obj->lcg + obj->xs;
}

///////////////////////////////////
///// 64-bit output functions /////
///////////////////////////////////

static uint64_t get_bits_u64(void *state)
{
    return superduper64_get_bits(state);
}


static uint64_t get_sum_u64(void *state, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += superduper64_get_bits(state);
    }
    return sum;
}

///////////////////////////////////
///// 32-bit output functions /////
///////////////////////////////////

static uint64_t get_bits_u32(void *state)
{
    return superduper64_get_bits(state) >> 32;
}


static uint64_t get_sum_u32(void *state, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += (superduper64_get_bits(state) >> 32);
    }
    return sum;
}

/////////////////////
///// Interface /////
/////////////////////

static void *create(const CallerAPI *intf)
{
    SuperDuper64State *obj = intf->malloc(sizeof(SuperDuper64State));
    obj->lcg = intf->get_seed64();
    do {
        obj->xs = intf->get_seed64();
    } while (obj->xs == 0);
    return obj;
}

static const char description[] =
"SuperDuper64: a 64-bit version of the combined generator by G.Marsaglia\n"
"The next param values are supported:\n"
"    u64 - full 64-bit output (default)\n"
"    u32 - return only upper 32 bits\n";

static const GeneratorParamVariant gen_list[] = {
    {"",      "SuperDuper64:u64", 64, default_create, get_bits_u64, get_sum_u64},
    {"u64",   "SuperDuper64:u64", 64, default_create, get_bits_u64, get_sum_u64},
    {"u32",   "SuperDuper64:u32", 32, default_create, get_bits_u32, get_sum_u32},
};


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
