/**
 * @file xtea.c
 * @brief An implementation of XTEA: a 64-bit block cipher with a 128-bit key.
 * @details XTEA is used as a "lightweight cryptography" for embedded system
 * but is rather slow (comparable to DES) on modern x86-64 processors. It is
 * suspectible to birthday paradox attack in CTR mode (fails the `birthday`
 * battery). Even in CBC mode it is prone to Sweet32 attack.
 *
 * References:
 *
 * - https://www.cix.co.uk/~klockstone/xtea.pdf
 * - https://www.cix.co.uk/~klockstone/teavect.htm
 * - https://tayloredge.com/reference/Mathematics/XTEA.pdf
 *
 * Results:
 *
 * - 4*2=8 rounds: fails `express`
 * - 5*2=10 rounds: passes `express`, passes `default`, fails `full` batteries.
 *   (`sumcollector` test)
 * - 6*2=12 rounds: passes `full` (tested only on `sumcollector`).
 *
 * @copyright Implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief XTEA PRNG state.
 */
typedef struct {
    uint64_t ctr;
    uint32_t key[4];
} XteaState;


static inline uint64_t get_bits_raw(void *state)
{
    static const uint32_t DELTA = 0x9e3779b9;
    XteaState *obj = state;
    uint32_t sum = 0, y, z;
    union {
        uint32_t v[2];
        uint64_t x;
    } out;
    out.x = obj->ctr++;
    y = out.v[0];
    z = out.v[1];
    for (int i = 0; i < 32; i++) {
        y += ( ((z<<4) ^ (z>>5)) + z) ^ (sum + obj->key[sum & 3]);
        sum += DELTA;
        z += ( ((y<<4) ^ (y>>5)) + y) ^ (sum + obj->key[(sum >> 11) & 3]);
    }
    out.v[0] = y;
    out.v[1] = z;
    return out.x;
}


static void *create(const CallerAPI *intf)
{
    XteaState *obj = intf->malloc(sizeof(XteaState));
    uint64_t s0 = intf->get_seed64();
    uint64_t s1 = intf->get_seed64();
    obj->key[0] = (uint32_t) s0;
    obj->key[1] = s0 >> 32;
    obj->key[2] = (uint32_t) s1;
    obj->key[3] = s1 >> 32;
    return (void *) obj;
}

static int run_self_test(const CallerAPI *intf)
{
    XteaState obj = {.ctr = 0x547571AAAF20A390,
        .key = {0x27F917B1, 0xC1DA8993, 0x60E2ACAA, 0xA6EB923D}};
    uint64_t u_ref = 0x0A202283D26428AF;
    uint64_t u = get_bits_raw(&obj);
    intf->printf("Results: out = %llX; ref = %llX\n",
        (unsigned long long) u,
        (unsigned long long) u_ref);
    return u == u_ref;    
}

MAKE_UINT64_PRNG("XTEA", run_self_test)
