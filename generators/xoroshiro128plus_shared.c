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
    uint64_t s[2];
} Xoroshiro128PlusState;


static uint64_t get_bits(void *state)
{
    Xoroshiro128PlusState *obj = state;
	const uint64_t s0 = obj->s[0];
	uint64_t s1 = obj->s[1];
	const uint64_t result = s0 + s1;
	s1 ^= s0;
	obj->s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
	obj->s[1] = rotl(s1, 37); // c
	return result;
}

static void *create(const CallerAPI *intf)
{
    Xoroshiro128PlusState *obj = intf->malloc(sizeof(Xoroshiro128PlusState));
    obj->s[0] = intf->get_seed64() | 0x1;
    obj->s[1] = intf->get_seed64() | 0x1;
    return (void *) obj;    
}

MAKE_UINT64_PRNG("xoroshiro128+", NULL)
