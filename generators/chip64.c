// https://github.com/tommyettinger/juniper/blob/main/src/main/java/com/github/tommyettinger/random/Chip32Random.java


#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
} Chip64State;

// 37 - by `a` output and hamming_distr from the full battery
// 7 - by rotl64(b, 7) + c output and hamming distr from the default battery
//     using the ++ increment instead of golden ratio
// 28 - same as 7 but to the fully assembled permutation
static inline uint64_t get_bits_raw(Chip64State *obj)
{
    const uint64_t a = obj->a, b = obj->b, c = obj->c, d = obj->d;
    obj->a = b + c;
    obj->b = d ^ a;
    obj->c = rotl64(b, 37);
    obj->d += 0x9E3779B97F4A7C15U;
    return rotl64(a, 28) ^ (rotl64(b, 7) + c);
}

static void *create(const CallerAPI *intf)
{
    Chip64State *obj = intf->malloc(sizeof(Chip64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    obj->d = intf->get_seed64();
    return obj;
}

MAKE_UINT64_PRNG("chip64", NULL)
