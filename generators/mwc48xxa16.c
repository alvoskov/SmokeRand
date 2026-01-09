/**
 * @file mwc48xxa16.c
 * @brief MWC48XXA16.
 * @details Multiply-with-carry PRNG.
 *
 * \f[
 * x_{n} = ax_{n - 2} + c \mod 2^16
 * \f]
 * 
 *
 * References:
 * 1. https://tom-kaitchuck.medium.com/designing-a-new-prng-1c4ffd27124d
 * 2. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 3. Sebastiano Vigna. MWC128. https://prng.di.unimi.it/MWC128.c
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC48XXA16 state. Cannot be initialized to (0, 0, 0) or to
 * (2^64 - 1, 2^16 - 1, carry_max). Default initialization is (seed, seed, 1)
 * as suggested by S. Vigna.
 */
typedef struct {
    uint16_t x[2];
    uint16_t c;
} Mwc48xxa16State;

static inline uint64_t get_bits_raw(Mwc48xxa16State *obj)
{
    static const uint32_t MWC_A1 = 52563U;
    uint32_t ans = 0;
    for (int i = 0; i < 2; i++) {
        uint32_t t = MWC_A1 * (uint32_t) obj->x[1];
        uint16_t ans16 = (uint16_t) ( (obj->x[1] ^ obj->x[0]) + (obj->c ^ (t >> 16)) );
        t += obj->c;
        obj->x[1] = obj->x[0];
        obj->x[0] = (uint16_t) t;
        obj->c = (uint16_t) (t >> 16);
        ans = (ans << 16) | ans16;
    }
    return ans;
}

static void Mwc48xxa16State_init(Mwc48xxa16State *obj, uint32_t seed)
{
    obj->x[0] = (uint16_t) seed;
    obj->x[1] = (uint16_t) (seed >> 16);
    obj->c = 1;
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}

static void *create(const CallerAPI *intf)
{
    Mwc48xxa16State *obj = intf->malloc(sizeof(Mwc48xxa16State));
    Mwc48xxa16State_init(obj, intf->get_seed32());
    return obj;
}


MAKE_UINT32_PRNG("Mwc48xxa16", NULL)
