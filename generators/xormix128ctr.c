/**
 * @file xormix128ctr.c
 * @brief xormix128ctr is a fast counter-based PRNG for 32-bit processors,
 * 128-bit seed/key and with 128-bit output block.
 * @details Its designed is based on an experimental ARX cipher suggested by
 * L. Hars and G. Petruska. The next modifications were made by A.L. Voskov
 * during the xormix128 design, they are intended to turn it into a fast
 * non-cryptographic 32-bit Counter-Based PRNG (CBPRNG):
 *
 * 1. Two different rotatations instead of one, dynamic indexing inside
 *    the key schedule was excluded.
 * 2. The key schedule is based on just scrambling the key with a combination
 *    of xorrot LFSR and a non-linear output function.
 * 3. Number of rounds were reduced, also xormix128ctr round corresponds to
 *    TWO rounds of the original cipher.
 *
 * Notes about rotations tuning:
 *
 * 1. They were manually tuned by means of hamming_distr test from default and
 *    full batteries of SmokeRand.
 * 2. 2 rounds: hamming_distr from full is ok (even without key addition)
 * 3. ctr[0] increment passes the entire "full" SmokeRand and >= 1 TiB
 *    in PractRand 0.96.
 *
 * WARNING! NOT FOR CRYPTOGRAPHY! Use only as a general purpose CBPRNG!
 *
 * References:
 *
 * 1. Hars L., Petruska G. Pseudorandom recursions II. // J Embedded Systems.
 *    2012, 1 (2012). https://doi.org/10.1186/1687-3963-2012-1
 * 2. Hars L., Petruska G. Pseudorandom Recursions: Small and Fast Pseudorandom
 *    Number Generators for Embedded Applications // J Embedded Systems 2007,
 *    098417 (2007). https://doi.org/10.1155/2007/98417
 * 3. Hars L. Hardware Bit-Mixers. Cryptology {ePrint} Archive, Paper 2017/084.
 *    https://eprint.iacr.org/2017/084
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t key[4];
    uint32_t ctr[4];
    uint32_t out[4];
    int pos;
} Xormix128CtrState;


static inline uint64_t get_bits_raw(Xormix128CtrState *obj)
{
    const int sh1 = 19, sh2 = 5;
    if (obj->pos == 4) {
        uint32_t x = obj->ctr[0], y = obj->ctr[1];
        uint32_t z = obj->ctr[2], w = obj->ctr[3];
        for (int i = 0; i < 3; i++) {
            x += rotl32(y ^ z ^ w, sh1) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh2) + obj->key[1];
            z += rotl32(w ^ x ^ y, sh1) + obj->key[2];
            w += rotl32(x ^ y ^ z, sh2) + obj->key[3];

            x += rotl32(y ^ z ^ w, sh2) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh1) + obj->key[2];
            z += rotl32(w ^ x ^ y, sh2) + obj->key[1];
            w += rotl32(x ^ y ^ z, sh1) + obj->key[3];
        }
        obj->out[0] = x; obj->out[1] = y;
        obj->out[2] = z; obj->out[3] = w;
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


void Xormix128CtrState_init(Xormix128CtrState *obj, const uint32_t *key)
{
    // Key mixer    
    uint32_t x = key[0], y = key[1], z = key[2], w = key[3];
    for (int i = 0; i < 64 + 4; i++) {
        const uint32_t x0 = x, w0 = w;
        x = x0 ^ y;
        y = z;
        z = x0 ^ w0;
        w = (x0 << 9) ^ z ^ rotl32(w0, 4) ^ rotl32(w0, 17);
        if (i >= 64) {
            uint32_t out = rotl32(69069U * x0, 5);
            out += (out * out | 0x4005);
            obj->key[i - 64] = out;
        }
    }
    // Reset counter
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    obj->pos = 4;
}



static void *create(const CallerAPI *intf)
{
    uint32_t k[4];
    Xormix128CtrState *obj = intf->malloc(sizeof(Xormix128CtrState));
    seeds_to_array_u32(intf, k, 4);
    Xormix128CtrState_init(obj, k);
    return obj;
}

MAKE_UINT32_PRNG("xormix128ctr", NULL)
