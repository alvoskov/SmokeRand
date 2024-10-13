/*
Daniel Lemire, Melissa E. O'Neill,
Xorshift1024*, xorshift1024+, xorshift128+ and xoroshiro128+ fail statistical tests for linearity,
Journal of Computational and Applied Mathematics,
Volume 350,
2019,
Pages 139-142,
ISSN 0377-0427,
https://doi.org/10.1016/j.cam.2018.10.019.
*/
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

static inline uint64_t rotl(const uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}


typedef struct {
    int p;
    uint64_t s[16];
} Xoroshiro1024StarState;


static uint64_t get_bits(void *state)
{
    Xoroshiro1024StarState *obj = state;
    const int q = obj->p;
    const uint64_t s0 = obj->s[obj->p = (obj->p + 1) & 15];
    uint64_t s15 = obj->s[q];
    const uint64_t result = s0 * 0x9e3779b97f4a7c13;

    s15 ^= s0;
    obj->s[q] = rotl(s0, 25) ^ s15 ^ (s15 << 27);
    obj->s[obj->p] = rotl(s15, 36);

    return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro1024StarState *obj = intf->malloc(sizeof(Xoroshiro1024StarState));
    obj->p = 0;
    for (size_t i = 0; i < 16; i++) {
        obj->s[i] = intf->get_seed64() | 0x1;
    }
    return (void *) obj;    
}

MAKE_UINT64_PRNG("xoroshiro1024*", NULL)
