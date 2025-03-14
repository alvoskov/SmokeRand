/*  Written in 2019 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES

WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define BUF_LEN (1000)
// You can play with this value on your architecture
#define XOROSHIRO128_UNROLL (4)

/* The current state of the generators. */

typedef struct {
    uint64_t s[2][XOROSHIRO128_UNROLL];
    uint64_t result[BUF_LEN];
    int pos;
} Xs128ppVecState;

static inline void next_block(Xs128ppVecState *obj)
{
 	uint64_t s1[XOROSHIRO128_UNROLL];
    for (int b = 0; b < BUF_LEN; b += XOROSHIRO128_UNROLL) {
        for (int i = 0; i < XOROSHIRO128_UNROLL; i++) {
            obj->result[b + i] = rotl64(obj->s[0][i] + obj->s[1][i], 17) + obj->s[0][i];
        }
        for (int i = 0; i < XOROSHIRO128_UNROLL; i++) {
            s1[i] = obj->s[0][i] ^ obj->s[1][i];
        }
        for (int i = 0; i < XOROSHIRO128_UNROLL; i++) {
            obj->s[0][i] = rotl64(obj->s[0][i], 49) ^ s1[i] ^ (s1[i] << 21);
        }
        for (int i = 0; i < XOROSHIRO128_UNROLL; i++) {
            obj->s[1][i] = rotl64(s1[i], 28);
        }
	}
}

static inline void *create(const CallerAPI *intf)
{
    Xs128ppVecState *obj = intf->malloc(sizeof(Xs128ppVecState));
    for (int i = 0; i < XOROSHIRO128_UNROLL; i++) {
        obj->s[0][i] = 1 << i;
        obj->s[1][i] = 0;
    }
    obj->pos = BUF_LEN;
    return obj;
}

static uint64_t get_bits_raw(void *state)
{
    Xs128ppVecState *obj = state;
    if (obj->pos >= BUF_LEN) {
        next_block(obj);
        obj->pos = 0;
    }
    return obj->result[obj->pos++];    
}

MAKE_UINT64_PRNG("xoroshiro128++VEC", NULL)
