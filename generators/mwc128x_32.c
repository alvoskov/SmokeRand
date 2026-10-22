/**
 * @file mwc128x_32.c
 * @details
 * Spectral test results are in agreement with [1] (see Table 8):
 *
 *     a = 4294967296; m = 340016513177126373822419941403394047999
 *       t = 2; mu = 0;       fd = 0;     log2(v) = 32; v2 = 18446744073709551617
 *       t = 3; mu = 0;       fd = .0005; log2(v) = 32; v2 = 18446744073709551617
 *       t = 4; mu = 4.9232;  fd = .8404; log2(v) = 32; v2 = 18417931412738062501
 *       t = 5; mu = 0;       fd = .0012; log2(v) = 16; v2 = 6395147593
 *       t = 6; mu = 0;       fd = .0232; log2(v) = 16; v2 = 6308101786
 *       t = 7; mu = .0002;   fd = .1846; log2(v) = 16; v2 = 6308101786
 *       t = 8; mu = 18.8204; fd = .8565; log2(v) = 16; v2 = 6301375294
 *     Merit = 0; Merit_H = .1690
 *
 * References:
 *
 * 1. Sebastiano Vigna. Scrambled Multiply-with-Carry Pseudorandom Number Generators
 *    https://arxiv.org/abs/2610.27313
 * 2. https://github.com/vigna/MWCPRNG
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x; ///< x_{n-3}
    uint32_t c; ///< carry
    uint32_t y; ///< x_{n-2}
    uint32_t z; ///< x_{n-1}
} MWC128X32State;


static inline uint64_t get_bits_raw(MWC128X32State *obj)
{
	const uint32_t out = obj->z ^ obj->c;
	const uint64_t t = 0xFFCCCC66U * (uint64_t)obj->x + obj->c;
	obj->x = obj->y;
	obj->y = obj->z;
	obj->z = (uint32_t) t;
	obj->c = (uint32_t) (t >> 32);
	return out;
}


static void *create(const CallerAPI *intf)
{
    MWC128X32State *obj = intf->malloc(sizeof(MWC128X32State));
    const uint64_t s0 = intf->get_seed64();
    const uint64_t s1 = intf->get_seed64();
    obj->x = (uint32_t) s0;
    obj->y = (uint32_t) (s0 >> 32);
    obj->z = (uint32_t) s1;
    obj->c = (uint32_t) (s1 >> 48) | 0x1000;
    return obj;
}

/**
 * @brief An internal self-test with values obtained from the Python 3.x
 * script (see the `..\misc\rwc.py` file)
 */
static int run_self_test(const CallerAPI *intf)
{
    MWC128X32State obj = {.x = 12345, .y = 67890, .z = 77777, .c = 1};
    const uint32_t z_ref = 0xFEAAC533;
    for (size_t i = 0; i < 100000; i++) {
        (void) get_bits_raw(&obj);
    }
    intf->printf("Result: %lX; reference value: %lX\n",
        (unsigned long) obj.z, (unsigned long) z_ref);
    return obj.z == z_ref;
}

MAKE_UINT32_PRNG("MWC128/X32", run_self_test)
