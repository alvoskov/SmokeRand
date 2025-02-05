/**
 * @file philox2x32_shared.c
 * @brief An implementation of Philox2x32x10 PRNG.
 * @details Philox PRNG is inspired by Threefish cipher but uses 32-bit
 * multiplication instead of cyclic shifts and uses reduced number of rounds.
 * Even 7 rounds is enough to pass BigCrush.
 *
 * References:
 *
 * 1. J. K. Salmon, M. A. Moraes, R. O. Dror and D. E. Shaw, "Parallel random
 *    numbers: As easy as 1, 2, 3," SC '11: Proceedings of 2011 International
 *    Conference for High Performance Computing, Networking, Storage and
 *    Analysis, Seattle, WA, USA, 2011, pp. 1-12.
 *    https://doi.org/10.1145/2063384.2063405.
 * 2. Random123: a Library of Counter-Based Random Number Generators
 *    https://github.com/girving/random123/blob/main/tests/kat_vectors
 *
 * @copyright Philox algorithm is proposed by J. K. Salmon, M. A. Moraes,
 * R. O. Dror and D. E. Shaw.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define Nw 2

PRNG_CMODULE_PROLOG

/////////////////////////////////////
///// Philox2x32 implementation /////
/////////////////////////////////////


typedef struct {
    uint32_t key; ///< Key
    uint32_t ctr[Nw]; ///< Counter ("plain text")
    uint32_t out[Nw]; ///< Output buffer
    size_t pos;
} Philox2x32State;


static void Philox2x32State_init(Philox2x32State *obj, const uint32_t key)
{
    for (size_t i = 0; i < Nw; i++) {
        obj->ctr[i] = 0;
    }
    obj->key = key;
    obj->pos = Nw;
}

static inline void philox_bumpkey(uint32_t *key)
{
    *key += 0x9E3779B9; // Golden ratio
}

static inline void philox_round(uint32_t *out, const uint32_t key)
{
    uint64_t mul0 = out[0] * 0xD256D193ull;
    uint32_t hi0 = mul0 >> 32, lo0 = (uint32_t) mul0;

    out[0] = hi0 ^ out[1] ^ key;
    out[1] = lo0;
}


EXPORT void Philox2x32State_block10(Philox2x32State *obj)
{
    uint32_t key, out[Nw];
    for (size_t i = 0; i < Nw; i++) {
        out[i] = obj->ctr[i];
    }
    key = obj->key;

    {                       philox_round(out, key); } // Round 0
    { philox_bumpkey(&key); philox_round(out, key); } // Round 1
    { philox_bumpkey(&key); philox_round(out, key); } // Round 2
    { philox_bumpkey(&key); philox_round(out, key); } // Round 3
    { philox_bumpkey(&key); philox_round(out, key); } // Round 4
    { philox_bumpkey(&key); philox_round(out, key); } // Round 5
    { philox_bumpkey(&key); philox_round(out, key); } // Round 6
    { philox_bumpkey(&key); philox_round(out, key); } // Round 7
    { philox_bumpkey(&key); philox_round(out, key); } // Round 8
    { philox_bumpkey(&key); philox_round(out, key); } // Round 9

    for (size_t i = 0; i < Nw; i++) {
        obj->out[i] = out[i];
    }
}

static inline void Philox2x32State_inc_counter(Philox2x32State *obj)
{
    uint64_t *ctr = (uint64_t *) obj->ctr;
    (*ctr)++;
}

///////////////////////////////
///// Internal self-tests /////
///////////////////////////////

static int self_test_compare(const CallerAPI *intf,
    const uint32_t *out, const uint32_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < Nw; i++) {
        intf->printf("%X ", out[i]);
        if (out[i] != ref[i])
            is_ok = 0;
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < Nw; i++) {
        intf->printf("%X ", ref[i]);
    }
    intf->printf("\n");
    return is_ok;
}


/**
 * @brief An internal self-test. Test vectors are taken
 * from Random123 library.
 */
static int run_self_test(const CallerAPI *intf)
{
    Philox2x32State obj;
    const uint32_t k0_m1 = 0xFFFFFFFF;
    const uint32_t ref_m1[2] = {0x2c3f628b, 0xab4fd7ad};
    const uint32_t k0_pi = 0x13198a2e;
    const uint32_t ref_pi[2] = {0xdd7ce038, 0xf62a4c12};

/*
philox2x32 10 00000000 00000000 00000000   ff1dae59 6cd10df2
philox2x32 10 ffffffff ffffffff ffffffff   
philox2x32 10 243f6a88 85a308d3 13198a2e   dd7ce038 f62a4c12
*/
    Philox2x32State_init(&obj, k0_m1);
    obj.ctr[0] = 0xFFFFFFFF; obj.ctr[1] = 0xFFFFFFFF;

    intf->printf("Philox2x32x10 ('-1' example)\n");
    Philox2x32State_block10(&obj);
    if (!self_test_compare(intf, obj.out, ref_m1)) {
        return 0;
    }

    Philox2x32State_init(&obj, k0_pi);
    obj.ctr[0] = 0x243f6a88; obj.ctr[1] = 0x85a308d3;
    intf->printf("Philox2x32x10 ('pi' example)\n");
    Philox2x32State_block10(&obj);
    if (!self_test_compare(intf,obj.out, ref_pi)) {
        return 0;
    }
    return 1;
}


/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////

static inline uint64_t get_bits_raw(void *state)
{
    Philox2x32State *obj = state;
    if (obj->pos >= Nw) {
        Philox2x32State_inc_counter(obj);
        Philox2x32State_block10(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    uint32_t k;
    Philox2x32State *obj = intf->malloc(sizeof(Philox2x32State));
    k = intf->get_seed32();
    Philox2x32State_init(obj, k);
    return (void *) obj;
}


MAKE_UINT32_PRNG("Philox2x32x10", run_self_test)
