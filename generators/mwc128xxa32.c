/**
 * @file mwc128xxa32.c
 * @brief MWC128XXA32.
 * @details Multiply-with-carry PRNG.
 *
 * \f[
 * x_{n} = ax_{n - 3} + c \mod 2^32
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
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC256 state. Cannot be initialized to (0, 0, 0, 0) or to
 * (2^64 - 1, 2^32 - 1, 2^32 - 1, 2^64 - 1). Default initialization
 * is (seed, seed, seed, 1) as suggested by S. Vigna.
 */
typedef struct {
    uint32_t x[3];
    uint32_t c;
} Mwc128xxa32State;

static inline uint64_t get_bits_raw(void *state)
{
    static const uint32_t MWC_A1 = 3487286589;
    Mwc128xxa32State *obj = state;
    uint64_t t = MWC_A1 * (uint64_t) obj->x[2];
    uint32_t ans = (uint32_t) ((obj->x[2] ^ obj->x[1]) + (obj->x[0] ^ (t >> 32)));
    t += obj->c;
    obj->x[2] = obj->x[1];
    obj->x[1] = obj->x[0];
    obj->x[0] = (uint32_t) t;
    obj->c = (uint32_t) (t >> 32);
    return ans;
}

static void Mwc128xxa32State_init(Mwc128xxa32State *obj, uint32_t s0, uint32_t s1)
{
    obj->x[0] = s0; obj->x[1] = s1;
    obj->x[2] = 0xcafef00d; obj->c = 0xd15ea5e5;
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}

static void *create(const CallerAPI *intf)
{
    Mwc128xxa32State *obj = intf->malloc(sizeof(Mwc128xxa32State));
    uint64_t s = intf->get_seed64();
    Mwc128xxa32State_init(obj, (uint32_t) s, (uint32_t) (s >> 32));
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Mwc128xxa32State obj;
    uint32_t u, u_ref = 0xc7357f43;
    Mwc128xxa32State_init(&obj, 12345, 67890);
    for (int i = 0; i < 1000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Result: %lX; reference value: %lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}


MAKE_UINT32_PRNG("Mwc128xxa32", run_self_test)
