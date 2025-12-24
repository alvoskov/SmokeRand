/**
 * @file seiran128.c
 * @brief Seiran128 v1 - pseudorandom number generator
 * @details Seiran128 is small and fast LFSR with output scrambler.
 * It has jump functions that can be found at:
 * 
 * - https://github.com/andanteyk/prng-seiran/blob/master/seiran128.c
 * @copyright
 * To the extent possible under law, the author has waived all copyright 
 * and related or neighboring rights to this software.
 * See: https://creativecommons.org/publicdomain/zero/1.0/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t s[2];
} Seiran128State;


static inline uint64_t get_bits_raw(Seiran128State *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1];
    uint64_t result = rotl64((s0 + s1) * 9, 29) + s0;
    obj->s[0] = s0 ^ rotl64(s1, 29);
    obj->s[1] = s0 ^ (s1 << 9);
    return result;
}


static void *create(const CallerAPI *intf)
{
    Seiran128State *obj = intf->malloc(sizeof(Seiran128State));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64();
    if (obj->s[0] == 0 && obj->s[1] == 0) {
        obj->s[0] = 0x12345678;
        obj->s[1] = 0x87654321;
    }
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[] = {
         0x8D4E3629D245305F, 0x941C2B08EB30A631,
         0x4246BDC17AD8CA1E, 0x5D5DA3E87E82EB7C
    };

    Seiran128State obj = {{0x6C64F673ED93B6CC, 0x97C703D5F6C9D72B}};
    int is_ok = 1;
    for (size_t i = 0; i < 4; i++) {
        const uint64_t u = get_bits_raw(&obj);
        intf->printf("Out = %16.16llX; ref = %16.16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("Seiran128", run_self_test)
