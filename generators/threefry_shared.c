/**
 * @file threefry_shared.c
 * @brief An implementation of Threefry4x64x72 and Threefry4x64x20 PRNGs.
 * They are based on Threefish cipher/CSPRNG, this cipher is almost identical
 * to the Threefry4x64x72 PRNG.
 *
 * @details Differences from Threefish:
 *
 * 1. Reduced number of rounds in the Threefry4x64x20.
 * 2. Tweak T is always set to {0, 0, 0}.
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
 * @copyright Threefry algorithm is proposed by J. K. Salmon, M. A. Moraes,
 * R. O. Dror and D. E. Shaw.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define Nw 4

PRNG_CMODULE_PROLOG

///////////////////////////////////
///// Threefry implementation /////
///////////////////////////////////

typedef struct {
    uint64_t k[Nw + 1]; // Key (+ extra word)
    uint64_t p[Nw]; // Counter ("plain text")
    uint64_t v[Nw]; // Output buffer
    size_t pos;
} Tf256State;


static void Tf256State_init(Tf256State *obj, const uint64_t *k)
{
    static const uint64_t C240 = 0x1BD11BDAA9FC1A22ULL;
    obj->k[Nw] = C240;
    for (size_t i = 0; i < Nw; i++) {
        obj->k[i] = k[i];
        obj->p[i] = 0;
        obj->k[Nw] ^= obj->k[i];
    }
}


static inline void mix4(uint64_t *x, int d1, int d2)
{
    // Not permuted
    x[0] = x[0] + x[1];
    x[2] = x[2] + x[3];
    // Permuted
    uint64_t x1 = x[1], x3 = x[3];
    x[3] = ( (x1 << d1) | (x1 >> (64 - d1)) ) ^ x[0];
    x[1] = ( (x3 << d2) | (x3 >> (64 - d2)) ) ^ x[2];
}

static inline void Tf256State_key_schedule(Tf256State *obj, uint64_t *out,
    size_t s, size_t s0, size_t s1, size_t s2, size_t s3)
{    
    out[0] += obj->k[s0];
    out[1] += obj->k[s1];
    out[2] += obj->k[s2];
    out[3] += obj->k[s3] + s;
}

static const int Rj0[8] = {14, 52, 23,  5,  25, 46, 58, 32};
static const int Rj1[8] = {16, 57, 40, 37,  33, 12, 22, 32};

#define ROUNDS_LOW(S, S0, S1, S2, S3) { \
    Tf256State_key_schedule(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4(v, Rj0[0], Rj1[0]); mix4(v, Rj0[1], Rj1[1]); \
    mix4(v, Rj0[2], Rj1[2]); mix4(v, Rj0[3], Rj1[3]); }

#define ROUNDS_HIGH(S, S0, S1, S2, S3) { \
    Tf256State_key_schedule(obj, v, (S), (S0), (S1), (S2), (S3)); \
    mix4(v, Rj0[4], Rj1[4]); mix4(v, Rj0[5], Rj1[5]); \
    mix4(v, Rj0[6], Rj1[6]); mix4(v, Rj0[7], Rj1[7]); }


EXPORT void Tf256State_block72(Tf256State *obj)
{
    static const int N_ROUNDS = 72;
    uint64_t v[Nw];
    for (size_t i = 0; i < Nw; i++) {
        v[i] = obj->p[i];
    }

    ROUNDS_LOW(0,  0, 1, 2, 3)   ROUNDS_HIGH(1,  1, 2, 3, 4)
    ROUNDS_LOW(2,  2, 3, 4, 0)   ROUNDS_HIGH(3,  3, 4, 0, 1)
    ROUNDS_LOW(4,  4, 0, 1, 2)   ROUNDS_HIGH(5,  0, 1, 2, 3)
    ROUNDS_LOW(6,  1, 2, 3, 4)   ROUNDS_HIGH(7,  2, 3, 4, 0) 
    ROUNDS_LOW(8,  3, 4, 0, 1)   ROUNDS_HIGH(9,  4, 0, 1, 2)
    ROUNDS_LOW(10, 0, 1, 2, 3)   ROUNDS_HIGH(11, 1, 2, 3, 4)
    ROUNDS_LOW(12, 2, 3, 4, 0)   ROUNDS_HIGH(13, 3, 4, 0, 1)
    ROUNDS_LOW(14, 4, 0, 1, 2)   ROUNDS_HIGH(15, 0, 1, 2, 3)
    ROUNDS_LOW(16, 1, 2, 3, 4)   ROUNDS_HIGH(17, 2, 3, 4, 0)

    // Output generation
    Tf256State_key_schedule(obj, v, N_ROUNDS / 4, 3, 4, 0, 1);
    for (size_t i = 0; i < Nw; i++) {
        obj->v[i] = v[i];
    }
}

EXPORT void Tf256State_block20(Tf256State *obj)
{
    static const int N_ROUNDS = 20;
    uint64_t v[Nw];
    for (size_t i = 0; i < Nw; i++) {
        v[i] = obj->p[i];
    }

    ROUNDS_LOW(0,  0, 1, 2, 3)   ROUNDS_HIGH(1,  1, 2, 3, 4)
    ROUNDS_LOW(2,  2, 3, 4, 0)   ROUNDS_HIGH(3,  3, 4, 0, 1)
    ROUNDS_LOW(4,  4, 0, 1, 2)

    // Output generation
    Tf256State_key_schedule(obj, v, N_ROUNDS / 4, 0, 1, 2, 3);
    for (size_t i = 0; i < Nw; i++) {
        obj->v[i] = v[i];
    }
}



static inline void Tf256State_inc_counter(Tf256State *obj)
{
    if (++obj->p[0] == 0) obj->p[1]++;
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
    Tf256State obj;
    static const uint64_t k0_m1[4] = {-1, -1, -1, -1};
    static const uint64_t ref72_m1[4] = {0x11518c034bc1ff4cull,
        0x193f10b8bcdcc9f7ull, 0xd024229cb58f20d8ull, 0x563ed6e48e05183full};
    static const uint64_t ref20_m1[4] = {0x29c24097942bba1bull,
        0x0371bbfb0f6f4e11ull, 0x3c231ffa33f83a1cull, 0xcd29113fde32d168ull};

    static const uint64_t k0_pi[4] = {0x452821e638d01377ull,
        0xbe5466cf34e90c6cull, 0xbe5466cf34e90c6c, 0xc0ac29b7c97c50ddull};
    static const uint64_t ref72_pi[4] = {0xacf412ccaa3b2270ull,
        0xc9e99bd53f2e9173ull, 0x43dad469dc825948ull, 0xfbb19d06c8a2b4dcull};
    static const uint64_t ref20_pi[4] = {0xa7e8fde591651bd9ull,
        0xbaafd0c30138319bull, 0x84a5c1a729e685b9ull, 0x901d406ccebc1ba4ull};

    Tf256State_init(&obj, k0_m1);
    obj.p[0] = -1; obj.p[1] = -1;
    obj.p[2] = -1; obj.p[3] = -1;

    intf->printf("Threefry4x64x72 ('-1' example)\n");
    Tf256State_block72(&obj);
    if (!self_test_compare(intf, obj.v, ref72_m1)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('-1' example)\n");
    Tf256State_block20(&obj);
    if (!self_test_compare(intf, obj.v, ref20_m1)) {
        return 0;
    }

    Tf256State_init(&obj, k0_pi);
    obj.p[0] = 0x243f6a8885a308d3; obj.p[1] = 0x13198a2e03707344;
    obj.p[2] = 0xa4093822299f31d0; obj.p[3] = 0x082efa98ec4e6c89;

    intf->printf("Threefry4x64x72 ('pi' example)\n");
    Tf256State_block72(&obj);
    if (!self_test_compare(intf, obj.v, ref72_pi)) {
        return 0;
    }
    intf->printf("Threefry4x64x20 ('pi' example)\n");
    Tf256State_block20(&obj);
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
    Tf256State *obj = state;
    if (obj->pos >= Nw) {
        Tf256State_inc_counter(obj);
        Tf256State_block20(obj);
        obj->pos = 0;
    }
    return obj->v[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
    uint64_t k[Nw];
    Tf256State *obj = intf->malloc(sizeof(Tf256State));
    for (size_t i = 0; i < Nw; i++) {
        k[i] = intf->get_seed64();
    }
    Tf256State_init(obj, k);
    return (void *) obj;
}


MAKE_UINT64_PRNG("Threefry4x64x20", run_self_test)
