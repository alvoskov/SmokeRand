/**
 * @brief jctr64 is a counter-based PRNG based on an experimental block
 * cipher by Bob Jenkins.
 * @details The original cipher is a 512-bit block ARX cipher based on 64-bit
 * arithmetics. The author didn't made a formal cryptoanalysis and positioned
 * it as "use this only if you are a fool". It is also used as during the
 * official ISAAC64 initialization procedure.
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
 * @copyright The original 512-bit ARX block cipher was designed
 * by Bob Jenkins and released as Public Domain.
 *
 * Conversion to `jctr64` counter-based PRNG:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Jctr64 counter-based PRNG state.
 * @details The state has the next layout:
 *
 *     | pi  key  pi  ctr_lo |
 *     | pi  key  pi  ctr_hi |
 */
typedef struct {
    uint64_t x[8];   ///< Working state
    uint64_t out[8]; ///< Output state
    size_t pos;
} Jctr64State;

/**
 * @brief Taken from ISAAC64 source code equipped with test vectors.
 */
static inline void jctr64_round(uint64_t *x)
{
    x[0] -= x[4]; x[5] ^= x[7] >> 9;  x[7] += x[0];
    x[1] -= x[5]; x[6] ^= x[0] << 9;  x[0] += x[1];
    x[2] -= x[6]; x[7] ^= x[1] >> 23; x[1] += x[2];
    x[3] -= x[7]; x[0] ^= x[2] << 15; x[2] += x[3];
    x[4] -= x[0]; x[1] ^= x[3] >> 14; x[3] += x[4];
    x[5] -= x[1]; x[2] ^= x[4] << 20; x[4] += x[5];
    x[6] -= x[2]; x[3] ^= x[5] >> 17; x[5] += x[6];
    x[7] -= x[3]; x[4] ^= x[6] << 14; x[6] += x[7];
}


void Jctr64State_block(Jctr64State *obj)
{
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] = obj->x[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    // 2 rounds for safety margin
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] += obj->x[i];
    }
}


void Jctr64State_init(Jctr64State *obj, const uint64_t *key, uint64_t ctr)
{
    obj->x[0] = 0x243F6A8885A308D3;
    obj->x[1] = key[0];
    obj->x[2] = 0x13198A2E03707344;
    obj->x[3] = ctr;
    obj->x[4] = 0xA4093822299F31D0;
    obj->x[5] = key[1];
    obj->x[6] = 0x082EFA98EC4E6C89;
    obj->x[7] = 0; // Reserved for an upper half of a counter
    obj->pos = 0;
    Jctr64State_block(obj);
}


static inline void Jctr64State_inc_counter(Jctr64State *obj)
{
    obj->x[3]++;
}


static inline uint64_t get_bits_raw(void *state)
{
    Jctr64State *obj = state;
    uint64_t x = obj->out[obj->pos++];
    if (obj->pos == 8) {
        Jctr64State_inc_counter(obj);
        Jctr64State_block(obj);
        obj->pos = 0;
    }
    return x;
}


static inline void *create(const CallerAPI *intf)
{
    uint64_t key[2];
    Jctr64State *obj = intf->malloc(sizeof(Jctr64State));
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Jctr64State_init(obj, key, 0);
    return obj;
}

MAKE_UINT64_PRNG("Jctr64", NULL)
