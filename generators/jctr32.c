/**
 * @brief jctr32 is a counter-based PRNG based on an experimental block
 * cipher by Bob Jenkins.
 * @details The original cipher is a 256-bit block ARX cipher based on 32-bit
 * arithmetics. The author didn't made a formal cryptoanalysis and positioned
 * it as "guaranteed to be stronger than rot-13".
 * 
 * Modifications made by A.L. Voskov:
 *
 * 1. Number of rounds was reduced to 6 (even 4 round pass SmokeRand `full`
 *    battery, and it is not for crypto anyway).
 * 2. It is converted to the construction similar to the stream cipher
 *    like ChaCha20.
 *
 * WARNING! NOT FOR CRYPTOGRAPHY! Use only as a general purpose CBPRNG!
 *
 * Refences:
 *
 * 1. https://burtleburtle.net/bob/crypto/myblock.html
 * 2. https://burtleburtle.net/bob/c/myblock.c
 * 3. https://oeis.org/A062964
 * 4. https://gist.github.com/retrohacker/e5fff72b7b75ee058924
 *
 * @copyright The original 256-bit ARX block cipher was designed
 * by Bob Jenkins and released as Public Domain.
 *
 * Conversion to `jctr32` counter-based PRNG:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Jctr32 counter-based PRNG state.
 * @details The state has the next layout:
 *
 *     | pi     key key ctr_lo |
 *     | ctr_hi pi  key key    |
 */
typedef struct {
    uint32_t x[8];   ///< Working state
    uint32_t out[8]; ///< Output state
    size_t pos;
} Jctr32State;


static inline void jctr32_round(uint32_t *x)
{
    x[0] -= x[4]; x[5] ^= x[7] >> 8;  x[7] += x[0];
    x[1] -= x[5]; x[6] ^= x[0] << 8;  x[0] += x[1];
    x[2] -= x[6]; x[7] ^= x[1] >> 11; x[1] += x[2];
    x[3] -= x[7]; x[0] ^= x[2] << 3;  x[2] += x[3];
    x[4] -= x[0]; x[1] ^= x[3] >> 6;  x[3] += x[4];
    x[5] -= x[1]; x[2] ^= x[4] << 4;  x[4] += x[5];
    x[6] -= x[2]; x[3] ^= x[5] >> 13; x[5] += x[6];
    x[7] -= x[3]; x[4] ^= x[6] << 13; x[6] += x[7];
}

void Jctr32State_block(Jctr32State *obj)
{
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] = obj->x[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    // 2 rounds for safety margin
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] += obj->x[i];
    }
}

void Jctr32State_init(Jctr32State *obj, const uint64_t *key, uint64_t ctr)
{
    obj->x[0] = 0x243F6A88;
    obj->x[1] = key[0];
    obj->x[2] = key[1];
    obj->x[3] = ctr;
    obj->x[4] = 0;
    obj->x[5] = 0x85A308D3;
    obj->x[6] = key[2];
    obj->x[7] = key[3];
    obj->pos = 0;
    Jctr32State_block(obj);
}


static inline void Jctr32State_inc_counter(Jctr32State *obj)
{
    if (obj->x[3]++) obj->x[4]++;
}


static inline uint64_t get_bits_raw(void *state)
{
    Jctr32State *obj = state;
    uint32_t x = obj->out[obj->pos++];
    if (obj->pos == 8) {
        Jctr32State_inc_counter(obj);
        Jctr32State_block(obj);
        obj->pos = 0;
    }
    return x;
}


static inline void *create(const CallerAPI *intf)
{
    uint64_t key[4];
    Jctr32State *obj = intf->malloc(sizeof(Jctr32State));
    uint64_t seed0 = intf->get_seed64();
    uint64_t seed1 = intf->get_seed64();
    key[0] = (uint32_t) seed0;
    key[1] = seed0 >> 32;
    key[2] = (uint32_t) seed1;
    key[3] = seed1 >> 32;
    Jctr32State_init(obj, key, 0);
    return obj;
}

MAKE_UINT32_PRNG("Jctr32", NULL)
