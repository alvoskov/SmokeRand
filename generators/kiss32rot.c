/**
 * @file kiss32rot.c
 * @brief 32-bit modification of KISS pseudorandom number generator
 * made by A.L. Voskov. Its period is shorted than in the original
 * KISS64 but performance is slightly higher.
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
    uint32_t mwc;  ///< MWC state
    uint32_t xr;   ///< XSH state
    uint32_t lcg;  ///< LCG state
} Kiss32RotState;


static inline uint64_t get_bits_raw(Kiss32RotState *obj)
{
    // MWC generator
    const uint32_t u = rotl32(obj->lcg, 3) + obj->xr + obj->mwc;
    // MWC generator
    obj->mwc = 63885U * (obj->mwc & 0xFFFF) + (obj->mwc >> 16);
    // XSH generator
    obj->xr ^= obj->xr << 1;
    obj->xr ^= rotl32(obj->xr, 9) ^ rotl32(obj->xr, 27);
    // LCG generator
    obj->lcg = 69069U * obj->lcg + 12345U;
    return u;
}


static void *create(const CallerAPI *intf)
{
    Kiss32RotState *obj = intf->malloc(sizeof(Kiss32RotState));
    obj->mwc = (intf->get_seed32() & 0x7fffffffU) | 1;
    obj->xr = intf->get_seed32();
    if (obj->xr == 0) {
        obj->xr = 0xDEADBEEFU;
    }
    obj->lcg = intf->get_seed32();
    return obj;
}



MAKE_UINT32_PRNG("KISS32-ROT", NULL)
