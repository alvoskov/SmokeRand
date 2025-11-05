/**
 * @file speck128.c
 * @brief Speck64/128 CSPRNG cross-platform implementation for 64-bit
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
 * - 6 rounds: passes `express`, fails `brief`, `default`
 * - 7 rounds: passes `default` and `full` tests
 *
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define NROUNDS 27

/**
 * @brief Speck64/128 state.
 */
typedef struct {
    uint32_t ctr[2]; ///< Counter 
    uint32_t out[2]; ///< Output buffer
    uint32_t keys[NROUNDS]; ///< Round keys
} Speck64x128State;


static inline void speck_round(uint32_t *x, uint32_t *y, const uint32_t k)
{
    *x = (rotr32(*x, 8) + *y) ^ k;
    *y = rotl32(*y, 3) ^ *x;
}


static void Speck64x128State_init(Speck64x128State *obj, const uint32_t *key, const CallerAPI *intf)
{
    uint32_t a, b, c, d;
    obj->ctr[0] = 0;
    obj->ctr[1] = 0;
    if (key == NULL) {
        a = intf->get_seed32(); b = intf->get_seed32();
        c = intf->get_seed32(); d = intf->get_seed32();
    } else {
        a = key[0]; b = key[1]; c = key[2]; d = key[3];
    }
    for (size_t i = 0; i < NROUNDS; ) {
        obj->keys[i] = a; speck_round(&b, &a, i++);
        obj->keys[i] = a; speck_round(&c, &a, i++);
        obj->keys[i] = a; speck_round(&d, &a, i++);
    }
}


static inline void Speck64x128State_block(Speck64x128State *obj)
{
    obj->out[0] = obj->ctr[0];
    obj->out[1] = obj->ctr[1];
    for (size_t i = 0; i < NROUNDS; i++) {
        speck_round(&(obj->out[1]), &(obj->out[0]), obj->keys[i]);
    }
}

static void *create(const CallerAPI *intf)
{
    Speck64x128State *obj = intf->malloc(sizeof(Speck64x128State));
    Speck64x128State_init(obj, NULL, intf);
    return (void *) obj;
}


/**
 * @brief Speck64/128 implementation.
 */
static inline uint64_t get_bits_raw(void *state)
{
    Speck64x128State *obj = state;    
    Speck64x128State_block(obj);
    if (++obj->ctr[0] == 0) ++obj->ctr[1];
    const uint64_t u0 = obj->out[0], u1 = obj->out[1];
    return u0 | (u1 << 32);
}

/**
 * @brief Internal self-test based on test vectors.
 */
int run_self_test(const CallerAPI *intf)
{
    const uint32_t key[] = {0x03020100, 0x0b0a0908, 0x13121110, 0x01b1a1918};
    const uint32_t ctr[] = {0x7475432d, 0x3b726574};
    const uint32_t out[] = {0x454e028b, 0x8c6fa548};
    Speck64x128State *obj = intf->malloc(sizeof(Speck64x128State));
    Speck64x128State_init(obj, key, intf);
    obj->ctr[0] = ctr[0]; obj->ctr[1] = ctr[1];
    Speck64x128State_block(obj);
    intf->printf("Output:    0x%8X 0x%8X\n", obj->out[0], obj->out[1]);
    intf->printf("Reference: 0x%8X 0x%8X\n", out[0], out[1]);
    int is_ok = obj->out[0] == out[0] && obj->out[1] == out[1];
    intf->free(obj);
    return is_ok;
}


MAKE_UINT64_PRNG("Speck64/128", run_self_test)
