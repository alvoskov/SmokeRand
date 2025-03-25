/**
 * @file threefry2x64.c
 * @brief An implementation of Threefry2x64x20 PRNGs. It is a simplified
 * version of Threefish with reduced size of blocks and rounds.
 *
 * @details Differences from Threefish:
 *
 * 1. Reduced number of rounds (20 instead of 72).
 * 2. Reduced block size: only 128 bit.
 * 3. Counter is used as a text.
 * 4. No XORing in output generation.
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
 * 3. https://www.schneier.com/academic/skein/threefish/
 * 4. https://pdebuyl.be/blog/2016/threefry-rng.html
 *
 * @copyright Threefish block cipher was developed by Bruce Schneier et al.
 * Threefry algorithm was proposed by J. K. Salmon, M. A. Moraes, R. O. Dror
 * and D. E. Shaw.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define Nw 2

///////////////////////////////////
///// Threefry implementation /////
///////////////////////////////////

typedef struct {
    uint64_t k[Nw + 1]; ///< Key schedule
    uint64_t p[Nw]; ///< Counter ("plain text")
    uint64_t v[Nw]; ///< Output buffer
    size_t pos;
} Threefry2x64State;


static void Threefry2x64State_init(Threefry2x64State *obj, const uint64_t *k)
{
    static const uint64_t C240 = 0x1BD11BDAA9FC1A22ULL;
    obj->k[Nw] = C240;
    for (size_t i = 0; i < Nw; i++) {
        obj->k[i] = k[i];
        obj->p[i] = 0;
        obj->k[Nw] ^= obj->k[i];
    }
}


static const int rot2x64[8] = {16, 42, 12, 31, 16, 32, 24, 21};


static inline void mix2(uint64_t *x, int d)
{
    x[0] += x[1];
    x[1] = rotl64(x[1], d);
    x[1] ^= x[0];
}

static inline void inject_key(uint64_t *out, const uint64_t *ks,
    size_t n, size_t i0, size_t i1)
{    
    out[0] += ks[i0];
    out[1] += ks[i1] + n;
}

#define MIX2_ROT_0_3 \
    mix2(v, rot2x64[0]); mix2(v, rot2x64[1]); \
    mix2(v, rot2x64[2]); mix2(v, rot2x64[3]);

#define MIX2_ROT_4_7 \
    mix2(v, rot2x64[4]); mix2(v, rot2x64[5]); \
    mix2(v, rot2x64[6]); mix2(v, rot2x64[7]);


EXPORT void Threefry2x64State_block20(Threefry2x64State *obj)
{
    uint64_t v[Nw];
    for (size_t i = 0; i < Nw; i++) {
        v[i] = obj->p[i];
    }
    // Initial key injection
    inject_key(v, obj->k, 0,  0, 1);
    // Rounds 0-3
    MIX2_ROT_0_3; inject_key(v, obj->k, 1,  1, 2);
    // Rounds 4-7
    MIX2_ROT_4_7; inject_key(v, obj->k, 2,  2, 0);
    // Rounds 8-11
    MIX2_ROT_0_3; inject_key(v, obj->k, 3,  0, 1);
    // Rounds 12-15
    MIX2_ROT_4_7; inject_key(v, obj->k, 4,  1, 2);
    // Rounds 16-19
    MIX2_ROT_0_3; inject_key(v, obj->k, 5,  2, 0);
    // Output generation
    for (size_t i = 0; i < Nw; i++) {
        obj->v[i] = v[i];
    }
}



static inline void Threefry2x64State_inc_counter(Threefry2x64State *obj)
{
    obj->p[0]++;
    /* obj->p[1] is reserved for thread number */
}

///////////////////////////////
///// Internal self-tests /////
///////////////////////////////


/**
 * @brief Comparison of vectors for internal self-tests.
 */
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
    Threefry2x64State obj;
    static const uint64_t k0_m1[4] = {-1, -1}, ctr_m1[4] = {-1, -1};
    static const uint64_t ref20_m1[4] = {0xe02cb7c4d95d277aull, 0xd06633d0893b8b68ull};

    static const uint64_t ctr_pi[4] = {0x243f6a8885a308d3ull, 0x13198a2e03707344ull};
    static const uint64_t k0_pi[4] = {0xa4093822299f31d0ull, 0x082efa98ec4e6c89ull};
    static const uint64_t ref20_pi[4] = {0x263c7d30bb0f0af1ull, 0x56be8361d3311526ull};

    intf->printf("Threefry2x64x20 ('-1' example)\n");
    Threefry2x64State_init(&obj, k0_m1);
    obj.p[0] = ctr_m1[0]; obj.p[1] = ctr_m1[1];
    Threefry2x64State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_m1)) {
        return 0;
    }

    intf->printf("Threefry2x64x20 ('pi' example)\n");
    Threefry2x64State_init(&obj, k0_pi);
    obj.p[0] = ctr_pi[0]; obj.p[1] = ctr_pi[1];
    Threefry2x64State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_pi)) {
        return 0;
    }
    return 1;
}


/////////////////////////////////////
///// Module external interface /////
/////////////////////////////////////

static inline uint64_t get_bits_raw(void *state)
{
    Threefry2x64State *obj = state;
    if (obj->pos >= Nw) {
        Threefry2x64State_inc_counter(obj);
        Threefry2x64State_block20(obj);
        obj->pos = 0;
    }
    return obj->v[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
    uint64_t k[Nw];
    Threefry2x64State *obj = intf->malloc(sizeof(Threefry2x64State));
    for (int i = 0; i < Nw; i++) {
        k[i] = intf->get_seed64();
    }
    Threefry2x64State_init(obj, k);
    return (void *) obj;
}


MAKE_UINT64_PRNG("Threefry2x64x20", run_self_test)
