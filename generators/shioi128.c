/**
 * @brief Shioi128 v1 - pseudorandom number generator
 *https://github.com/andanteyk/prng-shioi/blob/master/shioi128.c

 *To the extent possible under law, the author has waived all copyright 
 *and related or neighboring rights to this software.
 *See: https://creativecommons.org/publicdomain/zero/1.0/
*/
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t s[2];
} Shioi128State;


static inline uint64_t get_bits_raw(Shioi128State *obj)
{
    const uint64_t s0 = obj->s[0], s1 = obj->s[1];
    const uint64_t result = rotl64(s0 * 0xD2B74407B1CE6E93, 29) + s1;
    // Note: MUST use arithmetic right shift
    obj->s[0] = s1;
    obj->s[1] = (s0 << 2) ^ (uint64_t)((int64_t)s0 >> 19) ^ s1;
    return result;
}


static void *create(const CallerAPI *intf)
{
    Shioi128State *obj = intf->malloc(sizeof(Shioi128State));
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
        0xF8D7B7BA91C4D17A, 0xB053788D02AE0471,
        0xF6F7467B5C631C8A, 0x8F109E92A5905420
    };

    Shioi128State obj = {{0x6C64F673ED93B6CC, 0x97C703D5F6C9D72B}};
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

MAKE_UINT64_PRNG("Shioi128", run_self_test)
