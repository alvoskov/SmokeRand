/**
 * @file leptonflurry32x1.c
 * @brief Leptonflurry32x1 is a counter based PRNG developed by William Stafford
 * Parsons. It uses only 32-bit integers and is fairly slow at CPU (around 2-3 cpb).
 * 
 * References:
 *
 * 1. https://github.com/eightomic/leptonflurry/blob/master/leptonflurry.c
 * 2. https://eightomic.com/
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
 * @brief LeptonFlurry32x1 PRNG state.
 */
typedef struct {
    uint32_t ctr[2];
    uint32_t key[2];
} LeptonFlurry32x1;


static inline uint64_t get_bits_raw(LeptonFlurry32x1 *obj)
{
    uint32_t a = obj->key[0] + 11111U, b = obj->key[1] + 1111111111U;

    a += a << 15;
    a += obj->ctr[0] ^ b;
    a += a << 23;
    a ^= a >> 7;

    b += a + (a << 13);
    b += obj->ctr[1];
    b += b << 13;
    b ^= b >> 5;

    if (++obj->ctr[0] == 0) obj->ctr[1]++;

    a += b + (b << 19); b += a + (a << 23);
    a ^= b ^ (b >> 17); b += a + (a << 7);
    a ^= b ^ (b >> 13); b ^= a ^ (a >> 15);
    a += b + (b << 9);  b += a + (a << 17);
    return (a ^ (a >> 7)) + (b ^ (b >> 11));
}


static void *create(const CallerAPI *intf)
{
    LeptonFlurry32x1 *obj = intf->malloc(sizeof(LeptonFlurry32x1));
    seed64_to_2x32(intf, &obj->key[0], &obj->key[1]);
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    return obj;
}

MAKE_UINT32_PRNG("leptonflurry32x1", NULL)
