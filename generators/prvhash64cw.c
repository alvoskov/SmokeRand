/**
 * @file prvhash64cw.c
 * @brief prvhash64-core-weyl is based on chaotic PRNG developed
 * by Aleksey Vaneev.
 * @details It is a chaotic generator based on reversible mapping.
 * The "discrete Weyl sequence" (a counter) was added by A.L. Voskov
 * to provide a proven period not less than 2^64.
 *
 * References:
 *
 * 1. https://github.com/avaneev/prvhash
 *
 * @copyright The prvhash-core algorithm was developed by Aleksey Vaneev.
 *
 * "Weyl sequence" modification and implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t seed;
    uint64_t lcg;
    uint64_t hash;
    uint64_t w;
} PrvHashCore64State;


static inline uint64_t get_bits_raw(PrvHashCore64State *obj)
{
    obj->w += 0x9E3779B97F4A7C15;
    obj->seed *= obj->lcg * 2U + 1U;
	const uint64_t rs = rotl64(obj->seed, 32);
    obj->hash += rs + 0xAAAAAAAAAAAAAAAA;
    obj->lcg += obj->seed + obj->w;
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}


static void *create(const CallerAPI *intf)
{
    PrvHashCore64State *obj = intf->malloc(sizeof(PrvHashCore64State));
    obj->seed = intf->get_seed64();
    obj->lcg  = intf->get_seed64();
    obj->hash = intf->get_seed64();
    obj->w    = intf->get_seed64();
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[16] = {
        0x9E3779B97F4A7C15, 0x51F69051F92D937E,
        0x4DB41660104AE978, 0x56389E62B8669856,
        0x23F05EC6C6E77EBA, 0xEEA36F360823C2CE,
        0xF3FE74F5CC032A0B, 0xC275D1EA90BA88A6,
        0x7423628E4D909AEF, 0xFEFDE3EAA5E7D473,
        0x529C8D58F5F29196, 0xE2B1EFB63153680D,
        0x79FB838A4A43071D, 0xF60072CC4E611B06,
        0xFEE7E865F0FF326B, 0xC724B46C75A442DD
    };
    PrvHashCore64State obj = {0, 0, 0, 0};
    int is_ok = 1;
    for (size_t i = 0; i < 16; i++) {
        const uint64_t u = get_bits_raw(&obj);
        intf->printf("Out = %16.16" PRIX64 "; ref = %16.16" PRIX64 "\n",
            u, u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("prvhash-core64-weyl", run_self_test)
