// https://github.com/avaneev/prvhash
// 
/*
 * WARNING! It has no guaranteed minimal period, bad seeds are theoretically
 * possible. Usage of this generator for statistical, scientific and
 * engineering computations is strongly discouraged!
 */

#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t seed;
    uint64_t lcg;
    uint64_t hash;
} PrvHashCore64State;


static inline uint64_t get_bits_raw(PrvHashCore64State *obj)
{
    obj->seed *= obj->lcg * 2U + 1U;
	const uint64_t rs = rotl64(obj->seed, 32);
    obj->hash += rs + 0xAAAAAAAAAAAAAAAA;
    obj->lcg += obj->seed + 0x5555555555555555;
    obj->seed ^= obj->hash;
    return obj->lcg ^ rs;
}


static void *create(const CallerAPI *intf)
{
    PrvHashCore64State *obj = intf->malloc(sizeof(PrvHashCore64State));
    obj->seed = intf->get_seed64();
    obj->lcg  = intf->get_seed64();
    obj->hash = intf->get_seed64();
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[16] = {
        0x5555555555555555, 0x00000000DB6DB6DB,
        0x2492492192492492, 0x75D75DA0AAAAAA79,
        0x93064E905C127FE5, 0xE2585C9CA95671A3,
        0x28A44B31D428179E, 0x11B0B6A8D4BA3A73,
        0x195C6A4C23EE71AD, 0x5AA47859226BA23E,
        0xA7D42121695056D4, 0x142D7CD5D83342F2,
        0x3D42E83328C09C8F, 0x7E691C66BAC23222,
        0x82E1032F441F23A5, 0xA4BDE5C4A05E6256
    };
    PrvHashCore64State obj = {0, 0, 0};
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

MAKE_UINT64_PRNG("prvhash-core64", run_self_test)
