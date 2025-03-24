/**
 * @file mwc256xxa64.c
 * @brief MWC256XXA64.
 * @details Multiply-with-carry PRNG.
 *
 * \f[
 * x_{n} = ax_{n - 3} + c \mod 2^64
 * \f]
 * 
 *
 * References:
 * 1. Tom Kaitchuck. Designing a new PRNG.
 *    https://tom-kaitchuck.medium.com/designing-a-new-prng-1c4ffd27124d
 * 2. https://github.com/tkaitchuck/Mwc256XXA64
 * 3. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
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
 * (2^64 - 1, 2^64 - 1, 2^64 - 1, 2^64 - 1). Default initialization
 * is (seed, seed, seed, 1) as suggested by S. Vigna.
 */
typedef struct {
    uint64_t x[3];
    uint64_t c;
} Mwc256xxa64State;

//(X3 ^ X2) + (X1 ^ HI)
static inline uint64_t get_bits_raw(void *state)
{
    static const uint64_t MWC_A1 = 0xfeb344657c0af413;
    Mwc256xxa64State *obj = state;
//#ifdef UINT128_ENABLED
    __uint128_t t = MWC_A1 * (__uint128_t)obj->x[2];
    uint64_t ans = (obj->x[2] ^ obj->x[1]) + (obj->x[0] ^ (t >> 64));
    t += obj->c;
    obj->x[2] = obj->x[1];
    obj->x[1] = obj->x[0];
    obj->x[0] = t;
    obj->c = t >> 64;
//#else
//    uint64_t c_old = obj->c;
//    obj->x = unsigned_mul128(MWC_A1, obj->x, &obj->c);
//    obj->c += _addcarry_u64(0, obj->x, c_old, &obj->x);
//#endif
    return ans;
}

static void Mwc256xxa64State_init(Mwc256xxa64State *obj, uint64_t s0, uint64_t s1)
{
    obj->x[0] = s0; obj->x[1] = s1;
    obj->x[2] = 0xcafef00dd15ea5e5;
    obj->c = 0x14057B7EF767814F;
    for (int i = 0; i < 6; i++) {
        (void) get_bits_raw(obj);
    }
}


static void *create(const CallerAPI *intf)
{
    Mwc256xxa64State *obj = intf->malloc(sizeof(Mwc256xxa64State));
    Mwc256xxa64State_init(obj, intf->get_seed64(), intf->get_seed64());
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Mwc256xxa64State obj;
    uint64_t u, u_ref = 0x693f522810901b6; // From the original Rust implementation
    Mwc256xxa64State_init(&obj, 12345, 67890);
    for (int i = 0; i < 1000; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


MAKE_UINT64_PRNG("Mwc256xxa64", run_self_test)
