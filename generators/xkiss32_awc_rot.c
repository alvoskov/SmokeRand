// full, bigcrush, birthday, practrand >= 2 TiB
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x;
    uint32_t awc_x0;
    uint32_t awc_x1;
    uint32_t awc_c;
} Xkiss32AwcRotState;


static inline uint64_t get_bits_raw(Xkiss32AwcRotState *obj)
{
    // LFSR part
    obj->x ^= obj->x << 1;
    obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
    // AWC part
    uint32_t t = obj->awc_x0 + obj->awc_x1 + obj->awc_c;
    obj->awc_x1 = obj->awc_x0;
    obj->awc_c  = t >> 26;
    obj->awc_x0 = t & 0x3ffffff;
    // Output function
    uint32_t u = (obj->awc_x0 << 6) ^ (obj->awc_x1 * 29u);
    return obj->x ^ u;
}


static void *create(const CallerAPI *intf)
{
    Xkiss32AwcRotState *obj = intf->malloc(sizeof(Xkiss32AwcRotState));
    obj->x = intf->get_seed32();
    if (obj->x == 0) {
        obj->x = 0xDEADBEEF;
    }
    uint64_t seed = intf->get_seed64();
    obj->awc_x0 = (seed >> 32) & 0x3ffffff;
    obj->awc_x1 = seed & 0x3ffffff;
    obj->awc_c  = (obj->awc_x0 == 0 && obj->awc_x1 == 0) ? 1 : 0;
    return obj;
}


/**
 * @brief Test values were obtained from the code itself.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref = 0x453EFE6E;
    uint32_t u;
    Xkiss32AwcRotState obj = {
        .x  = 12345678,
        .awc_x0 = 3, .awc_x1 = 2, .awc_c = 1};
    for (long i = 0; i < 1000000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%lX; reference: 0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("XKISS32/AWC/ROT", run_self_test)
