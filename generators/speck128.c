/**
 * @file speck128.c
 * @brief Speck128/128 CSPRNG cross-platform implementation for 64-bit
 * processors. Its period is \f$ 2^{129} \f$. Performance is about
 * 3.1 cpb on Intel(R) Core(TM) i5-11400H 2.70GHz.
 *
 * References:
 *
 * 1. Ray Beaulieu, Douglas Shors et al. The SIMON and SPECK Families
 *    of Lightweight Block Ciphers // Cryptology ePrint Archive. 2013.
 *    Paper 2013/404. https://ia.cr/2013/404
 * 2. Ray Beaulieu, Douglas Shors et al. SIMON and SPECK implementation guide
 *    https://nsacyber.github.io/simon-speck/implementations/ImplementationGuide1.1.pdf
 * 3. Colin Josey. Reassessing the MCNP Random Number Generator. Technical
 *    Report LA-UR-23-25111. 2023. Los Alamos National Laboratory (LANL),
 *    Los Alamos, NM (United States) https://doi.org/10.2172/1998091
 *
 * Rounds:
 *
 * - 8 rounds: passes `brief`, `default`, fails `full` (mainly `hamming_ot_long`)
 * - 9 rounds: passes `full` battery.
 *
 * - 8 rounds: passes SmallCrush, fails PractRand at 8 GiB
 * - 9 rounds: passes Crush and BigCrush, fails PractRand at ???
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define NROUNDS 32

/**
 * @brief Speck128 state.
 */
typedef struct {
    uint64_t ctr[2]; ///< Counter
    uint64_t out[2]; ///< Output buffer
    uint64_t keys[NROUNDS]; ///< Round keys
    unsigned int pos;
} Speck128State;


static inline void speck_round(uint64_t *x, uint64_t *y, const uint64_t k)
{
    *x = (rotr64(*x, 8) + *y) ^ k;
    *y = rotl64(*y, 3) ^ *x;
}


static void Speck128State_init(Speck128State *obj, const uint64_t *key)
{
    obj->ctr[0] = 0;
    obj->ctr[1] = 0;
    obj->keys[0] = key[0];
    obj->keys[1] = key[1];
    uint64_t a = obj->keys[0], b = obj->keys[1];
    for (size_t i = 0; i < NROUNDS - 1; i++) {
        speck_round(&b, &a, i);
        obj->keys[i + 1] = a;
    }    
    obj->pos = 2;
}

static inline void Speck128State_block(Speck128State *obj)
{
    obj->out[0] = obj->ctr[0];
    obj->out[1] = obj->ctr[1];
    for (size_t i = 0; i < NROUNDS; i++) {
        speck_round(&(obj->out[1]), &(obj->out[0]), obj->keys[i]);
    }
}

static void *create(const CallerAPI *intf)
{
    Speck128State *obj = intf->malloc(sizeof(Speck128State));
    uint64_t key[2];
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Speck128State_init(obj, key);
    return (void *) obj;
}


/**
 * @brief Speck128/128 implementation.
 */
static inline uint64_t get_bits_raw(void *state)
{
    Speck128State *obj = state;
    if (obj->pos == 2) {
        Speck128State_block(obj);
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

/**
 * @brief Internal self-test based on test vectors.
 */
int run_self_test(const CallerAPI *intf)
{
    const uint64_t key[] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    const uint64_t ctr[] = {0x7469206564616d20, 0x6c61766975716520};
    const uint64_t out[] = {0x7860fedf5c570d18, 0xa65d985179783265};
    Speck128State *obj = intf->malloc(sizeof(Speck128State));
    Speck128State_init(obj, key);
    obj->ctr[0] = ctr[0]; obj->ctr[1] = ctr[1];
    Speck128State_block(obj);
    intf->printf("Output:    0x%16llX 0x%16llX\n", obj->out[0], obj->out[1]);
    intf->printf("Reference: 0x%16llX 0x%16llX\n", out[0], out[1]);
    int is_ok = obj->out[0] == out[0] && obj->out[1] == out[1];
    intf->free(obj);
    return is_ok;
}


MAKE_UINT64_PRNG("Speck128", run_self_test)
