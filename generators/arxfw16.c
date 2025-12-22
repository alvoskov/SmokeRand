/**
 * @file arxfw16.c
 * @brief ARX-FW-16 experimental chaotic generator.
 * @details It was made as a toy generator to make a scaled down version of
 * arxfw64 PRNG suitable for 16-bit processors such as Intel 8086.
 * The "FW" means "Feystel-Weyl"
 *
 * WARNING! The minimal guaranteed period is only 2^16, the average period is
 * small is only about 2^47, bad seeds are theoretically possible. Usage of
 * this generator for statistical, scientific and engineering computations is
 * strongly discouraged!
 *
 * Versions with `++` and `Weyl` behave reasonably well in empirical testing:
 *
 * - TestU01: BigCrush
 * - SmokeRand: full, 64-bit birthday paradox test
 * - PractRand 0.94: fail at 16 TiB
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arxfw16 PRNG state.
 */
typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t w;
} Arxfw16State;

static inline uint16_t get_bits16(Arxfw16State *obj)
{
    static const uint16_t inc = 0x9E37;
    uint16_t a = obj->a, b = obj->b;
    b = (uint16_t) (b + obj->w);    
    a = (uint16_t) (a + (rotl16(b, 3)  ^ rotl16(b, 8) ^ b));
    b = (uint16_t) (b ^ (rotl16(a, 15) + rotl16(a, 8) + a));
    obj->a = b;
    obj->b = a;
    obj->w = (uint16_t) (obj->w + inc);
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw16State *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Arxfw16State *obj = intf->malloc(sizeof(Arxfw16State));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint16_t) seed;
    obj->b = (uint16_t) (seed >> 16);
    obj->w = (uint16_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw16", NULL)
