/**
 * @file wich2006.c
 * @brief Wichmann-Hill generator (2006 version).
 * @details This implementation is based on integer arithmetics and returns
 * 32-bit unsigned integers instead of single-precision floats. It passes
 * SmokeRand test batteries.
 *
 * References:
 *
 * 1. B.A. Wichmann, I.D. Hill. Generating good pseudo-random numbers //
 *    Computational Statistics & Data Analysis. 2006. V. 51. N 3. P.1614-1622.
 *    https://doi.org/10.1016/j.csda.2006.05.019.
 *
 * @copyright The algorithm was suggested by B.A. Wichmann and I.D. Hill.
 * 
 * Implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[4];
} Wich2006State;

static const uint64_t
    WH06_A0 = 11600, WH06_MOD0 = 2147483579,
    WH06_A1 = 47003, WH06_MOD1 = 2147483543,
    WH06_A2 = 23000, WH06_MOD2 = 2147483423,
    WH06_A3 = 33000, WH06_MOD3 = 2147483123;

static uint64_t get_bits_raw(void *state)
{
    Wich2006State *obj = state;
    // Update generator state
    uint32_t *s = obj->s;
    s[0] = (uint32_t) ((WH06_A0 * s[0]) % WH06_MOD0);
    s[1] = (uint32_t) ((WH06_A1 * s[1]) % WH06_MOD1);
    s[2] = (uint32_t) ((WH06_A2 * s[2]) % WH06_MOD2);
    s[3] = (uint32_t) ((WH06_A3 * s[3]) % WH06_MOD3);
    // Output function
    uint64_t s0 = ((uint64_t) s[0] << 32) / WH06_MOD0;
    uint64_t s1 = ((uint64_t) s[1] << 32) / WH06_MOD1;
    uint64_t s2 = ((uint64_t) s[2] << 32) / WH06_MOD2;
    uint64_t s3 = ((uint64_t) s[3] << 32) / WH06_MOD3;
    return (s0 + s1 + s2 + s3) & 0xFFFFFFFF;
}

static void *create(const CallerAPI *intf)
{
    Wich2006State *obj = intf->malloc(sizeof(Wich2006State));
    uint64_t s1 = intf->get_seed64();
    uint64_t s2 = intf->get_seed64();
    obj->s[0] = (uint32_t) (1 + (s1 % (WH06_MOD0 - 1)));
    obj->s[1] = (uint32_t) (1 + ((s1 >> 32) % (WH06_MOD1 - 1)));
    obj->s[2] = (uint32_t) (1 + (s2 % (WH06_MOD2 - 1)));
    obj->s[3] = (uint32_t) (1 + ((s2 >> 32) % (WH06_MOD3 - 1)));
    return obj;
}

MAKE_UINT32_PRNG("Wich2006", NULL)
