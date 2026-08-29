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
    uint32_t out[2];
    int pos;
} LeptonFlurry32x2State;


static void LeptonFlurry32x2State_block(LeptonFlurry32x2State *obj)
{
    uint32_t a = obj->ctr[0] + obj->ctr[1] + obj->key[0] + 111111111U;
    uint32_t b = obj->ctr[2] + obj->ctr[3] + obj->key[1];

    a += (a << 9)  ^ obj->ctr[2] ^ (a >> 5) ^ obj->ctr[3] ^ obj->key[1];
    b += (b << 13) ^ obj->ctr[0] ^ (b >> 9) ^ obj->ctr[1] ^ obj->key[0] ^ a;

    a += (b + (b << 9)) ^ (b >> 7);
    b += (a + (a << 7)) ^ (a >> 5);
    a += (b << 13) + (b ^ (b >> 11));
    b += (a << 11) + (a ^ (a >> 9));
    a = (a ^ (a >> 15)) + (b << 17) + (b ^ (b >> 11));
    obj->out[0] = a ^ b;
    obj->out[1] = (a << 15) + (a ^ (a >> 17)) + b;
}


static uint64_t get_bits_raw(LeptonFlurry32x2State *obj)
{
    if (obj->pos == 2) {
        LeptonFlurry32x2State_block(obj);
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    LeptonFlurry32x2State *obj = intf->malloc(sizeof(LeptonFlurry32x2State));
    obj->key[0] = intf->get_seed32();
    obj->key[1] = intf->get_seed32();
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    LeptonFlurry32x2State_block(obj);
    obj->pos = 0;
    return obj;
}

MAKE_UINT32_PRNG("leptonflurry32x2", NULL)
