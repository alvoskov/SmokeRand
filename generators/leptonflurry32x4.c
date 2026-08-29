/**
 * @file leptonflurry32x2.c
 * @brief Leptonflurry32x2 is a counter based PRNG developed by William Stafford
 * Parsons. It uses only 32-bit integers.
 * 
 * References:
 *
 * 1. https://github.com/eightomic/leptonflurry/blob/master/leptonflurry.c
 * 2. https://eightomic.com/
 *
 * gap_inv1024, hamming_distr failures
 *
 * @copyright LeptonFlurry PRNG family was developed by
 * William Stafford Parsons.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LeptonFlurry32x2 PRNG state.
 */
typedef struct {
    uint32_t ctr[4];
    uint32_t key[2];
    uint32_t out[4];
    int pos;
} LeptonFlurry32x4State;


static inline uint32_t sc(uint32_t x)
{
    return x ^ rotl32(x, 11) ^ rotl32(x, 27);
}

static void LeptonFlurry32x4State_block(LeptonFlurry32x4State *obj)
{
    uint32_t a = obj->ctr[0] + obj->ctr[1] + obj->key[0] + 111111111U;
    uint32_t b = obj->ctr[2] + obj->ctr[3] + obj->key[1];

    a += (a << 9)  ^ obj->ctr[2] ^ (a >> 5) ^ obj->ctr[3] ^ obj->key[1];
    b += (b << 13) ^ obj->ctr[0] ^ (b >> 9) ^ obj->ctr[1] ^ obj->key[0] ^ a;
    a += (b + (b << 7)) ^ (b >> 5);
    b += (a + (a << 9)) ^ (a >> 7);
    a += (b << 11) + (b ^ (b >> 9));
    uint32_t c = a;
    b += (a << 13) + (a ^ (a >> 11));
    uint32_t d = b;
    a = (a ^ (a >> 17)) + (b << 15) + (b ^ (b >> 13));
    obj->out[0] = a ^ b;
    obj->out[1] = (a << 15) + (a ^ (a >> 17)) + b;
    b += a ^ (a >> 15);
    obj->out[2] = b + c;
    obj->out[3] = b ^ d;
}

static uint64_t get_bits_raw(LeptonFlurry32x4State *obj)
{
    if (obj->pos == 4) {
        LeptonFlurry32x4State_block(obj);
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    LeptonFlurry32x4State *obj = intf->malloc(sizeof(LeptonFlurry32x4State));
    obj->key[0] = intf->get_seed32();
    obj->key[1] = intf->get_seed32();
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    LeptonFlurry32x4State_block(obj);
    obj->pos = 0;
    return obj;
}

MAKE_UINT32_PRNG("leptonflurry32x4", NULL)
