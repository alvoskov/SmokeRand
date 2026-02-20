/**
 * @file xsadd.c
 * @brief XSADD LFSR-based generator with a simple output scrambler.
 * @details
 *
 * Fails the linear complexity/matrix rank test in the lowest bit(s), also
 * fails `hamming_distr` and `hamming_ot_values` tests from the `default`
 * battery. Speed is around 1.5-2 cpb, i.e. it may be slower than hardware
 * accelerated AES or ChaCha8.
 *
 * References:
 *
 * 1. https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/XSADD/
 * 2. https://github.com/MersenneTwister-Lab/XSadd
 *
 * @copyright xsadd algorithm was suggested by Mutsuo Saito and 
 * Makoto Matsumoto.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief XSADD pseudorandom number generator state.
 */
typedef struct {
    uint32_t x[4];
} XsAddState;


static inline uint64_t get_bits_raw(XsAddState *obj)
{
    uint32_t *x = obj->x;
    uint32_t t = x[0];
    t ^= t << 15;
    t ^= t >> 18;
    t ^= x[3] << 11;
    x[0] = x[1];
    x[1] = x[2];
    x[2] = x[3];
    x[3] = t;
    return x[2] + x[3];
}

static void XsAddState_warmup(XsAddState *obj)
{
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    XsAddState *obj = intf->malloc(sizeof(XsAddState));
    seeds_to_array_u32(intf, obj->x, 4);
    if (obj->x[0] == 0 && obj->x[1] == 0 && obj->x[2] == 0 && obj->x[3] == 0) {
        obj->x[0] = 0x12345678;
        obj->x[1] = 0x87654321;
        obj->x[2] = 0xABCDEF00;
        obj->x[3] = 0xFEBCBA00;
    }
    XsAddState_warmup(obj);
    return obj;
}

/**
 * @brief An internal self test that uses test vectors from the reference
 * implementation.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[16] = {
        1823491521, 1658333335, 1467485721,   45623648,
        3336175492, 2561136018,  181953608,  768231638, 
        3747468990,  633754442, 1317015417, 2329323117, 
         688642499, 1053686614, 1029905208, 3711673957
    };
    // Seeds were generated from the 1234 seed using the default
    // initialization procedure.
    XsAddState obj = {.x = {3112782482, 3394581006, 3085658808, 77654758}};
    XsAddState_warmup(&obj);
    int is_ok = 1;
    for (int i = 0; i < 16; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(&obj);
        intf->printf("Out = %lu; Ref = %lu\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}


MAKE_UINT32_PRNG("xsadd", run_self_test)
