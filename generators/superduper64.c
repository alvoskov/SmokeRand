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
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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

static inline uint64_t superduper64_get_bits(void *state)
{
    SuperDuper64State *obj = state;
    obj->lcg = 6906969069 * obj->lcg + 1234567;
    obj->xs ^= (obj->xs << 13);
    obj->xs ^= (obj->xs >> 17);
    obj->xs ^= (obj->xs << 43);
    return obj->lcg + obj->xs;
}

static void *superduper64_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    SuperDuper64State *obj = intf->malloc(sizeof(SuperDuper64State));
    obj->lcg = intf->get_seed64();
    do {
        obj->xs = intf->get_seed64();
    } while (obj->xs == 0);
    (void) gi;
    return (void *) obj;
}

static void superduper64_free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    intf->free(state);
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
    (void) intf;
    return NULL;
}

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = NULL;
    gi->create = superduper64_create;
    gi->free = superduper64_free;
    gi->self_test = NULL;
    gi->parent = NULL;
    if (!intf->strcmp(param, "u64") || !intf->strcmp(param, "")) {
        gi->name = "SuperDuper64:u64";
        gi->nbits = 64;
        gi->get_bits = get_bits_u64;
        gi->get_sum = get_sum_u64;
    } else if (!intf->strcmp(param, "u32")) {
        gi->name = "SuperDuper64:u32";
        gi->nbits = 32;
        gi->get_bits = get_bits_u32;
        gi->get_sum = get_sum_u32;
    } else {
        gi->name = "SuperDuper:unknown";
        gi->nbits = 64;
        gi->create = default_create;
        gi->free = default_free;
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
