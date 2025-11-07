/**
 * @file philox.c
 * @brief An implementation of Philox4x64x10 PRNG.
 * @details Philox PRNG is inspired by Threefish cipher but uses 128-bit
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
#include "smokerand/int128defs.h"

#define Nw 4

PRNG_CMODULE_PROLOG

/////////////////////////////////
///// Philox implementation /////
/////////////////////////////////


typedef struct {
    uint64_t key[Nw / 2]; ///< Key
    uint64_t ctr[Nw]; ///< Counter ("plain text")
    uint64_t out[Nw]; ///< Output buffer
    size_t pos;
} PhiloxState;


static void PhiloxState_init(PhiloxState *obj, const uint64_t *key)
{
    for (size_t i = 0; i < Nw / 2; i++) {
        obj->key[i] = key[i];
    }    
    for (size_t i = 0; i < Nw; i++) {
        obj->ctr[i] = 0;
    }
    obj->pos = Nw;
}

static inline void philox_bumpkey(uint64_t *key)
{
    key[0] += 0x9E3779B97F4A7C15ull; // Golden ratio
    key[1] += 0xBB67AE8584CAA73Bull; // sqrt(3) - 1
}



static inline void philox_round(uint64_t *out, const uint64_t *key)
{
    uint64_t hi0, lo0, hi1, lo1;
    lo0 = unsigned_mul128(out[0], 0xD2E7470EE14C6C93ull, &hi0);
    lo1 = unsigned_mul128(out[2], 0xCA5A826395121157ull, &hi1);
    out[0] = hi1 ^ out[1] ^ key[0]; out[1] = lo1;
    out[2] = hi0 ^ out[3] ^ key[1]; out[3] = lo0;
}


EXPORT void PhiloxState_block10(PhiloxState *obj)
{
    uint64_t key[Nw], out[Nw];
    for (size_t i = 0; i < Nw; i++) {
        out[i] = obj->ctr[i];
    }
    for (size_t i = 0; i < Nw / 2; i++) {
        key[i] = obj->key[i];
    }


    {                      philox_round(out, key); } // Round 0
    { philox_bumpkey(key); philox_round(out, key); } // Round 1
    { philox_bumpkey(key); philox_round(out, key); } // Round 2
    { philox_bumpkey(key); philox_round(out, key); } // Round 3
    { philox_bumpkey(key); philox_round(out, key); } // Round 4
    { philox_bumpkey(key); philox_round(out, key); } // Round 5
    { philox_bumpkey(key); philox_round(out, key); } // Round 6
    { philox_bumpkey(key); philox_round(out, key); } // Round 7
    { philox_bumpkey(key); philox_round(out, key); } // Round 8
    { philox_bumpkey(key); philox_round(out, key); } // Round 9

    for (size_t i = 0; i < Nw; i++) {
        obj->out[i] = out[i];
    }
}

static inline void PhiloxState_inc_counter(PhiloxState *obj)
{
    if (++obj->ctr[0] == 0) obj->ctr[1]++;
}

///////////////////////////////
///// Internal self-tests /////
///////////////////////////////

static int self_test_compare(const CallerAPI *intf,
    const uint64_t *out, const uint64_t *ref)
{
    intf->printf("OUT: ");
    int is_ok = 1;
    for (size_t i = 0; i < Nw; i++) {
        intf->printf("%llX ", out[i]);
        if (out[i] != ref[i])
            is_ok = 0;
    }
    intf->printf("\n");
    intf->printf("REF: ");
    for (size_t i = 0; i < Nw; i++) {
        intf->printf("%llX ", ref[i]);
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
    PhiloxState obj;
    static const uint64_t
        k0_m1[2] = {UINT64_MAX, UINT64_MAX},
        ref_m1[4] = {0x87b092c3013fe90bull, 0x438c3c67be8d0224ull,
            0x9cc7d7c69cd777b6ull, 0xa09caebf594f0ba0ull},
        k0_pi[2] = {0x452821e638d01377ull, 0xbe5466cf34e90c6cull},
        ref_pi[4] = {0xa528f45403e61d95ull, 0x38c72dbd566e9788ull,
            0xa5a1610e72fd18b5ull, 0x57bd43b5e52b7fe6ull};

    PhiloxState_init(&obj, k0_m1);
    obj.ctr[0] = UINT64_MAX; obj.ctr[1] = UINT64_MAX;
    obj.ctr[2] = UINT64_MAX; obj.ctr[3] = UINT64_MAX;

    intf->printf("Philox4x64x10 ('-1' example)\n");
    PhiloxState_block10(&obj);
    if (!self_test_compare(intf, obj.out, ref_m1)) {
        return 0;
    }

    PhiloxState_init(&obj, k0_pi);
    obj.ctr[0] = 0x243f6a8885a308d3; obj.ctr[1] = 0x13198a2e03707344;
    obj.ctr[2] = 0xa4093822299f31d0; obj.ctr[3] = 0x082efa98ec4e6c89;

    intf->printf("Philox4x64x10 ('pi' example)\n");
    PhiloxState_block10(&obj);
    if (!self_test_compare(intf, obj.out, ref_pi)) {
        return 0;
    }
    return 1;
}


/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////

static inline uint64_t get_bits_raw(void *state)
{
    PhiloxState *obj = state;
    if (obj->pos >= Nw) {
        PhiloxState_inc_counter(obj);
        PhiloxState_block10(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    uint64_t k[Nw / 2];
    PhiloxState *obj = intf->malloc(sizeof(PhiloxState));
    for (size_t i = 0; i < Nw / 2; i++) {
        k[i] = intf->get_seed64();
    }
    PhiloxState_init(obj, k);
    return (void *) obj;
}

MAKE_UINT64_PRNG("Philox4x64x10", run_self_test)
