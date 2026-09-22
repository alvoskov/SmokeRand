/**
 * @file arata.c
 * @brief arata pseudorandom number generator, 64-bit version.
 *
 * @copyright The arata PRNG family was designed by K--Aethiax
 * https://github.com/eternal-io/arata/tree/master
 *
 * Reentrant modification partially based on the original public
 * domain K--Aethiax code:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief arata
 */
typedef struct {
    uint64_t w;
    uint64_t a;
    uint64_t x;
    uint64_t domain_id;
} ArataState;


static inline uint64_t moremur(uint64_t x)
{
    x ^= x >> 27;
    x *= 0x3c79ac492ba7b653ULL;
    x ^= x >> 33;
    x *= 0x1c69b3f74ac4ae35ULL;
    x ^= x >> 27;
    return x;
}

static inline uint64_t get_bits_raw(ArataState *obj)
{
    const uint64_t out = obj->x + obj->a;
    const uint64_t tmp = obj->w ^ obj->x ^ obj->domain_id;
    obj->x  = obj->a ^ rotl64(obj->a, 17) ^ rotl64(obj->a, 42);
    obj->a += tmp;
    obj->w += 0x9e3779b97f4a7c15ULL;
    return out;
}

static void ArataState_mix(ArataState *obj)
{
    (void) get_bits_raw(obj);
    (void) get_bits_raw(obj);
    const uint64_t w = moremur(obj->w ^ (~obj->a & obj->x));
    const uint64_t a = moremur(obj->a ^ (~obj->x & obj->w));
    const uint64_t x = moremur(obj->x ^ (~obj->w & obj->a));
    obj->w = w, obj->a = a, obj->x = x;
}


static void *create(const CallerAPI *intf)
{
    ArataState *obj = intf->malloc(sizeof(ArataState));
    obj->w = intf->get_seed64();
    obj->a = intf->get_seed64();
    obj->x = intf->get_seed64();
    ArataState_mix(obj);
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    ArataState obj = {0, 1, 2, 3456};
    const uint64_t u_ref = 0x879fdc6e8bc1fb7fULL;
    uint64_t u;
    ArataState_mix(&obj);
    for (int i = 0; i < 8; i++) {
        u = get_bits_raw(&obj);
    }
    intf->printf("%llX %llX\n",
        (unsigned long long) u, (unsigned long long) u_ref);
    return u == u_ref;
}

MAKE_UINT64_PRNG("arata", run_self_test)
