/**
 * @file kiss64rot.c
 * @brief 64-bit modification of KISS pseudorandom number generator
 * made by A.L. Voskov. Its period is shorted than in the original
 * KISS64 but performance is slightly higher.
 *
 * References to the original KISS64 by G. Marsaglia:
 *
 * - https://groups.google.com/g/comp.lang.fortran/c/qFv18ql_WlU
 * - https://www.thecodingforums.com/threads/64-bit-kiss-rngs.673657/
 * - https://ssau.ru/pagefiles/sbornik_pit_2021.pdf
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief KISS64-rot PRNG state.
 * @details Contains states of 3 PRNG: LCG, XSH, MWC.
 * z, w and jsr musn't be initialized with zeros.
 */
typedef struct {
    uint64_t mwc;  ///< MWC state
    uint64_t xr;   ///< XSH state
    uint64_t lcg;  ///< LCG state
} Kiss64RotState;


static inline uint64_t get_bits_raw(Kiss64RotState *obj)
{
    // MWC generator
    const uint64_t u = rotl64(obj->lcg, 5) + obj->xr + obj->mwc;
    // MWC generator
    obj->mwc = 0xff676488U * (obj->mwc & 0xFFFFFFFF) + (obj->mwc >> 32);
    // XSH generator
    obj->xr ^= obj->xr << 5;
    obj->xr ^= rotl64(obj->xr, 13) ^ rotl64(obj->xr, 47);
    // LCG generator
    obj->lcg = 6906969069ULL * obj->lcg + 1234567ULL;
    return u;
}


static void *create(const CallerAPI *intf)
{
    Kiss64RotState *obj = intf->malloc(sizeof(Kiss64RotState));
    obj->mwc = (intf->get_seed64() & 0x7fffffffffffffffU) | 1;
    obj->xr = intf->get_seed64();
    if (obj->xr == 0) {
        obj->xr = 0xCAFEBABEDEADBEEFU;
    }
    obj->lcg = intf->get_seed64();
    return obj;
}



MAKE_UINT64_PRNG("KISS64-ROT", NULL)
