/**
 * @file aesdec2.c
 * @brief aesdec2 is a counter-based PRNG that scrambles two 64-bit counters
 * ("discrete Weyl sequences")
 * @details Developed by camel-cdr (Olaf Bernstein):
 *
 * - https://www.reddit.com/r/RNG/comments/1qa6okr/3_instruction_prng_passes_practrand_2tb_weyl/
 * - https://github.com/camel-cdr
 *
 * Citation:
 *
 * I was experimenting with the AES-NI instructions yesterday and came up with
 * an extremely simple and fast PRNG that passes PractRand up to >2TB (after which
 * I stopped the test):
 *
 *    vpaddq  xmm1, xmm1, xmm0
 *    vaesdec xmm3, xmm1, xmm2
 *    vaesdec xmm3, xmm3, xmm2
 *
 * Or in C:
 *
 *    #include <immintrin.h>
 *    #include <stdint.h>
 *    #include <stddef.h>
 *    void
 *    aesdec2(__m128i *out, size_t n, uint64_t seed[2])
 *    {
 *        __m128i weyl, inc, mix;
 *        inc = _mm_set_epi64x(0xb5ad4eceda1ce2a9, 0x278c5a4d8419fe6b);
 *        weyl = mix = _mm_set_epi64x(seed[0], seed[1]);
 *        while (n--) {
 *            *out++ = _mm_aesdec_si128(_mm_aesdec_si128(weyl, mix), mix);
 *            weyl = _mm_add_epi64(weyl, inc);
 *        }
 *    }
 *
 * The increments for discrete Weyl sequences were taken from Wydinski's
 * "Middle Squares" PRNG (https://arxiv.org/pdf/1704.00358)
 *
 * @copyright aesdec2 PRNG was developed by camel-cdr (Olaf Bernstein)
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

#if defined(__AES__) || (defined(_MSC_VER) && !defined(__clang__) && defined(__AVX2__))
    #define AESNI_ENABLED
    #include "smokerand/x86exts.h"
#endif


typedef struct {
    uint64_t seed[2];
    uint64_t weyl[2];
    uint64_t out[2];
    int pos;
} AesDec2State;


static inline uint64_t get_bits_raw(AesDec2State *obj)
{
    if (obj->pos == 2) {
#ifdef AESNI_ENABLED
        const __m128i inc = _mm_set_epi64x(
            (long long int) 0xb5ad4eceda1ce2a9U,
            (long long int) 0x278c5a4d8419fe6bU
        );
        __m128i weyl       = _mm_loadu_si128((__m128i *) (void *) obj->weyl);
        const __m128i seed = _mm_loadu_si128((__m128i *) (void *) obj->seed);
        const __m128i out  = _mm_aesdec_si128(_mm_aesdec_si128(weyl, seed), seed);
        _mm_storeu_si128((__m128i *) (void *) obj->out, out);
        // Counter ("discrete Weyl sequence") increment
        weyl = _mm_add_epi64(weyl, inc);
        _mm_storeu_si128((__m128i *) (void *) obj->weyl, weyl);
#endif
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


static void *create(const CallerAPI *intf)
{
#ifdef AESNI_ENABLED
    AesDec2State *obj = intf->malloc(sizeof(AesDec2State));
    obj->seed[0] = intf->get_seed64();
    obj->seed[1] = intf->get_seed64();
    obj->weyl[0] = intf->get_seed64();
    obj->weyl[1] = intf->get_seed64();
    obj->pos = 2;
    return obj;
#else
    (void) intf;
    return NULL;
#endif
}


static int run_self_test(const CallerAPI *intf)
{
#ifdef AESNI_ENABLED
    AesDec2State *obj = intf->malloc(sizeof(AesDec2State));
    const uint64_t x_ref = 0xDD3C03C442E5ABD;
    uint64_t x;
    obj->seed[0] = 0x123456789ABCDEF0;
    obj->seed[1] = 0xDEADBEEFCAFEBABE;
    obj->weyl[0] = obj->seed[0];
    obj->weyl[1] = obj->seed[1];
    obj->pos = 2;
    for (int i = 0; i < 20000; i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("x = %llX; x_ref = %llX\n",
        (unsigned long long) x, (unsigned long long) x_ref);
    intf->free(obj);

    return (x == x_ref) ? 1 : 0;
#else
    intf->printf("This PRNG requires AESNI instructions set\n");
    return 0;
#endif
}


MAKE_UINT64_PRNG("aesdec2", run_self_test)
