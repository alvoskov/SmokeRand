/**
 * @file mwc64x.c
 * @brief MWC64X - 64-bit PRNG based on MWC method.
 * @details Multiply-with-carry PRNG with a simple output function x ^ c.
 * Has a period about 2^63. Passes SmallCrush, Crush and BigCrush tests.
 *
 * This PRNG is was proposed by David B. Thomas. MWC itself is invented
 * by G. Marsaglia. The A0 multiplier was changed by A.L. Voskov using
 * spectral test from TAOCP and SmallCrush for the version without XORing.
 *
 * Without XORing this PRNG fails "birthdayspacing t=3 (N12)" test from
 * Crush battery. The similar problem was noticed by S.Vigna in MWC128,
 * and he proposed a similar (but not the same) solution with XORing.
 *
 * References:
 * 1. David B. Thomas. The MWC64X Random Number Generator.
 *    https://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html
 * 2. G. Marsaglia "Multiply-With-Carry (MWC) generators" (from DIEHARD
 *    CD-ROM) https://www.grc.com/otg/Marsaglia_MWC_Generators.pdf
 * 3. https://github.com/lpareja99/spectral-test-knuth
 * 4. Sebastiano Vigna. Scrambled Multiply-with-Carry Pseudorandom Number
 *    Generators https://arxiv.org/abs/2610.27313
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
 * @brief MWC64X state.
 */
typedef struct {
    uint64_t data;
} MWC64XState;

static inline uint32_t MWC64XState_next(MWC64XState *obj, uint32_t a)
{
    const uint32_t c = (uint32_t) (obj->data >> 32);
    const uint32_t x = (uint32_t) (obj->data & 0xFFFFFFFF);
    obj->data = (uint64_t)a * (uint64_t)x + c;
    return x ^ c;    
}

//////////////////////////

static inline uint64_t get_bits_v1_raw(MWC64XState *obj)
{
    return MWC64XState_next(obj, 0xff676488); // 2^32 - 10001272
}

MAKE_GET_BITS_WRAPPERS(v1)

//////////////////////////

static inline uint64_t get_bits_mnem_raw(MWC64XState *obj)
{
    return MWC64XState_next(obj, 4027340273); // 40273_40273
}

MAKE_GET_BITS_WRAPPERS(mnem)

//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    MWC64XState *obj = intf->malloc(sizeof(MWC64XState));
    // Seeding: prevent (0,0) and (?,0xFFFF)
    do {
        obj->data = intf->get_seed64() << 1;
    } while (obj->data == 0);
    return obj;
}

static const GeneratorParamVariant gen_list[] = {
    {"",     "MWC64X:v1",   32, default_create, get_bits_v1,   get_sum_v1},
    {"v1",   "MWC64X:v1",   32, default_create, get_bits_v1,   get_sum_v1},
    {"mnem", "MWC64X:mnem", 32, default_create, get_bits_mnem, get_sum_mnem},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"The MWC64X generator. The next param values are supported:\n"
"  v1   - a = 0xff676488 = 2^32 - 10001272 (default version)\n"
"  mnem - a = 40273_40273 (mnemonic)\n";

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
