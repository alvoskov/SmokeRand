/**
 * @file mad0.c
 * @brief MaD0-ex nonlinear generator with the added linear part, modified mixer
 * and a simplified initialization procedure.
 * @details The next simplifications and changes were made by A.L. Voskov
 * in this implementation.
 *
 * In both `original` and `ext` versions:
 *
 * 1. A complex initialization procedure was replaced into the generator
 *    warmup: 32 iterations for block generations are used instead of MARC-bb.
 *    Also s buffer is filled by a slightly scrambled LCG output.
 *
 * In the `ext` version:
 *
 * 1. Discrete Weyl sequence added, it provides a minimial period at least
 *    2^64. The PRNG runs correctly even if its initial state is all zeros.
 * 2. An extra rotation was added to prevent failure of the matrix rank tests.
 *    That failure was reproduced even without Weyl sequence and for the PRNG
 *    initialized by solely `get_seed64` seeder.
 *
 * References:
 *
 * 1. Jie Li, Jianliang Zheng, and Paula Whitlock. MaD0: An ultrafast nonlinear
 *    pseudorandom number generator // ACM Trans. Model. Comput. Simul. 2016.
 *    V. 26. N. 2, Article 13. https://doi.org/10.1145/2856693
 *
 * @copyright The MaD0 algorithm was developed by Jie Li, Jianliang Zheng and
 * Paula Whitlock. The implementation for SmokeRand with the simplified 
 * initialization procedure and added linear part was made by A.L. Voskov.
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MaD0 pseudorandom number generator state: a version with
 * the linear part based on the discrete Weyl sequence.
 */
typedef struct {
    uint64_t a; ///< State: register a (nonlinear part)
    uint64_t b; ///< State: register b (nonlinear part)
    uint64_t c; ///< State: register c (nonlinear part)
    uint64_t d; ///< State: register d (nonlinear part)
    uint64_t w; ///< Discrete Weyl sequence: linear part
    uint64_t s[32]; ///< State: table (nonlinear part)
    uint64_t t[64]; ///< Output buffer
    int pos; ///< Current position in the output buffer
} MaD0State;

/**
 * @brief Generates a block of 64 unsigned 64-bit integers.
 * @details An extended mixer that has an extended period and doesn't fail
 * the `matrixrank_4096_low8` statistical test.
 */
static inline void MaD0State_ext_block(MaD0State *obj)
{
    uint64_t ta = (obj->a += obj->c + obj->w);
    uint64_t tb = (obj->b += obj->d);
    obj->w += 0x9E3779B97F4A7C15; // Added by A.L. Voskov
    for (int i = 0; i < 32; i++) {
        obj->t[2*i] = (obj->c ^= obj->s[i] + obj->a);
        obj->c += ta ^ rotl64(tb, 17); // rotl added by A.L. Voskov
        obj->d ^= obj->c + obj->b;
        ta = rotl64(ta, 3);
        obj->d += ta ^ tb;
        obj->t[2*i + 1] = obj->s[i] = obj->d;
        tb = rotr64(tb, 5);
    }
}


static inline void MaD0State_block(MaD0State *obj)
{
    uint64_t ta = (obj->a += obj->c);
    uint64_t tb = (obj->b += obj->d);
    for (int i = 0; i < 32; i++) {
        obj->t[2*i] = (obj->c ^= obj->s[i] + obj->a);
        obj->c += ta ^ tb;
        obj->d ^= obj->c + obj->b;
        ta = rotl64(ta, 3);
        obj->d += ta ^ tb;
        obj->t[2*i + 1] = obj->s[i] = obj->d;
        tb = rotr64(tb, 5);
    }
}


static inline uint64_t get_bits_orig_raw(MaD0State *obj)
{
    const uint64_t u = obj->t[obj->pos];
    if (obj->pos == 0) {
        obj->pos = 64;
        MaD0State_block(obj);
    }
    obj->pos--;
    return u;
}

MAKE_GET_BITS_WRAPPERS(orig)

static inline uint64_t get_bits_ext_raw(MaD0State *obj)
{
    const uint64_t u = obj->t[obj->pos];
    if (obj->pos == 0) {
        obj->pos = 64;
        MaD0State_ext_block(obj);
    }
    obj->pos--;
    return u;
}

MAKE_GET_BITS_WRAPPERS(ext)


static void *create(const CallerAPI *intf)
{
    MaD0State *obj = intf->malloc(sizeof(MaD0State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    obj->d = intf->get_seed64();
    obj->w = intf->get_seed64();
    uint64_t lcg = obj->w;
    for (unsigned int i = 0; i < 32; i++) {
        lcg = 6906969069U * lcg + 1234567U;
        obj->s[i] = lcg ^ (lcg >> 32);
    }
    for (int i = 0; i < 8; i++) {
        obj->s[4*i]     ^= obj->a;
        obj->s[4*i + 1] ^= obj->b;
        obj->s[4*i + 2] ^= obj->c;
        obj->s[4*i + 3] ^= obj->d;
    }
    // Warmup
    for (int i = 0; i < 32; i++) {
        MaD0State_block(obj);
    }
    obj->pos = 63;
    return obj;
}

static const GeneratorParamVariant gen_list[] = {
    {"",         "mad0:ext",  64, default_create, get_bits_ext,  get_sum_ext},
    {"ext",      "mad0:ext",  64, default_create, get_bits_ext,  get_sum_ext},
    {"original", "mad0:orig", 64, default_create, get_bits_orig, get_sum_orig},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"MaD0 is an experimental nonlinear generator.\n"
"The next param values are supported:\n"
"  ext      - A modification made by A.L. Voskov (default version)\n"
"  original - The original version (fails the matrixrank_4096_low8 test)\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = NULL;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
