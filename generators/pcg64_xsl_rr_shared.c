/**
 * 128-bit LCG with the PCG-XSL-RR output function (`pcg64`)
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t get_bits_raw(void *state)
{
    Lcg128State *obj = state;
    // Just ordinary 128-bit LCG
    (void) Lcg128State_a128_iter(state, 0x2360ED051FC65DA4, 0x4385DF649FCCF645, 1);    
    // Output XSL RR function
#ifdef UINT128_ENABLED
    uint64_t x_lo = (uint64_t) obj->x, x_hi = obj->x >> 64;
#else
    uint64_t x_lo = obj->x_low, x_hi = obj->x_high;
#endif
    unsigned int rot = x_hi >> 58; // 64 - 6
    uint64_t xsl = x_hi ^ x_lo;
    return (xsl << (64 - rot)) | (xsl >> rot);
}


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
    return (void *) obj;
}


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
#ifdef UINT128_ENABLED
    Lcg128State obj = {.x = 1234567890};
#else
    Lcg128State obj = { .x_low = 1234567890, .x_high = 0 };
#endif
    uint64_t u, u_ref = 0x8DE320BB801095E2;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("Lcg128Xsl64", run_self_test)
