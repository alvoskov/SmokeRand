/**
 * @file tt800.c
 * @brief TT800 is a twisted GFSR generator (essentially LFSR), its period
 * is \f$ 2^{800} - 1 \f$.
 * @details It has a maximal period for LFSR, also there are two versions
 * of TT800: from 1994 and from 1996 (with the improved tempering). The
 * original 1994 version fails some statistical tests such as `hamming_distr`
 * and `gap16_count0`. This program implements the later version that
 * fails only the linear complexity/matrix rank tests from SmokeRand `full`
 * battery.
 *
 * The LFSR core of TT800 can be represented as the next recurrent formula:
 *
 * \f[
 * x_{i+n} = x_{i+m} \oplus x_{i} A
 * \f]
 *
 * where x is the the bit ROW vector and A is the bit matrix, \f$\oplus\f$
 * is bitwise XOR. The entire structure strongly resembles xorshift128
 * or xorshift160.
 *
 * Note: if you want to verify the period of TT800 then undefine the
 * `TT800_USE_BUF` macro: it will enable the slower `NOBUF` version that
 * doesn't use `pos` variable.
 *
 * References:
 *
 * 1. Matsumoto M. and Kurita Y. Twisted GFSR generators //
 *    ACM Trans. Model. Comput. Simul. 1992. V 2. N 3. P. 179-194.
 *    https://doi.org/10.1145/146382.146383
 * 2. Matsumoto M. and Kurita Y. Twisted GFSR generators II //
 *    ACM Trans. Model. Comput. Simul. 1994. V.4. N 3. P. 254–266
 *    https://doi.org/10.1145/189443.189445
 * 3. https://docs.rs/crate/gsl_rust/0.7.4/source/gsl/rng/tt.c
 * 4. https://github.com/ampl/gsl/blob/master/rng/tt.c
 *
 * @copyright
 * TT800 PRNG was designed by Matsumoto M. and Kurita Y.
 *
 * Reentrant C99 implementation for SmokeRand:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define TT800_N 25
#define TT800_M 7
#define TT800_USE_BUF

#ifdef TT800_USE_BUF
#define TT800_NAME "TT800"
#else
#define TT800_NAME "TT800_NOBUF"
#endif

typedef struct {
    uint32_t x[TT800_N];
#ifdef TT800_USE_BUF
    int pos;
#endif
} TT800State;


static inline uint32_t tt800_mat(uint32_t x)
{
    return ((x & 0x1) == 0) ? (x >> 1) : ((x >> 1) ^ 0x8ebfd028); // a
}

/**
 * @brief Expands a 64-bit seed using the scrambled upper 32 bits of
 * 64-bit Klimov-Shamir "crazy" T-function TF0. It allows to avoid zeroland
 * and low quality output at the beginning of the output sequence.
 */
static void TT800State_init(TT800State *obj, uint64_t seed)
{
    uint64_t u = seed;
    for (int i = 0; i < TT800_N; i++) {
        u += u * u | 0x40000005;
        const uint32_t u32 = (uint32_t) (u >> 32);
        obj->x[i] = u32 ^ rotl32(u32, 7) ^ rotl32(u32, 23);
    }
#ifdef TT800_USE_BUF
    obj->pos = TT800_N;
#endif
}


static inline uint32_t TT800State_next(TT800State *obj)
{
#ifdef TT800_USE_BUF
    uint32_t *x = obj->x;
    // Refresh the generator state (generate N words)
    if (obj->pos == TT800_N) {
        for (int i = 0; i < TT800_N - TT800_M; i++) {
            x[i] = x[i + TT800_M] ^ tt800_mat(x[i]);
        }
        for (int i = TT800_N - TT800_M; i < TT800_N; i++) {
            x[i] = x[i + (TT800_M - TT800_N)] ^ tt800_mat(x[i]);
        }
        obj->pos = 0;
    }
    uint32_t u = x[obj->pos++];
#else
    uint32_t u = obj->x[TT800_M] ^ tt800_mat(obj->x[0]);
    for (int i = 0; i < TT800_N - 1; i++) {
        obj->x[i] = obj->x[i + 1];
    }
    obj->x[TT800_N - 1] = u;
#endif
    // Tempering
    u ^= (u << 7)  & 0x2b5b2500; // s, b
    u ^= (u << 15) & 0xdb8b0000; // t, c
    u ^= u >> 16; // Added in the 1996 version
    return u;
}


static inline uint64_t get_bits_raw(TT800State *obj)
{
    return TT800State_next(obj);
}


static void *create(const CallerAPI *intf)
{
    TT800State *obj = intf->malloc(sizeof(TT800State));
    TT800State_init(obj, intf->get_seed64());
    return obj;
}

/**
 * @brief The internal self-test that uses the seeds from the original
 * work with TT800 description and reference outputs - from the reference
 * program. Note that linear complexity of the output sequence is 800.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_init[TT800_N] = {
        0x95f24dab, 0x0b685215, 0xe76ccae7, 0xaf3ec239, 0X715fad23,
        0x24a590ad, 0x69e4b5ef, 0xbf456141, 0x96bc1b7b, 0xa7bdf825,
        0xc1de75b7, 0x8858a9c9, 0x2da87693, 0xb657f9dd, 0xffdc8a9f,
        0x8121da71, 0x8b823ecb, 0x885d05f5, 0x4e20cd47, 0x5a9ad5d9,
        0x512c0c03, 0xea857ccd, 0x4cc1d30f, 0x8891a8a1, 0xa6b7aadb
    };
    static const uint32_t u_ref[16] = {
        0x491AB319, 0xB367D019, 0xEBBC9786, 0x745E18D8,
        0x5238A508, 0xAF8B123B, 0xD907F0EC, 0x29AF915C,
        0xE2FCD75B, 0xA31F5424, 0x158D79F3, 0x262EB7FD,
        0x9687AB2F, 0x21AF956B, 0x31F07D90, 0x50E428CD
    };
    TT800State *obj = create(intf);
    for (int i = 0; i < TT800_N; i++) {
        obj->x[i] = x_init[i];
    }
#ifdef TT800_USE_BUF
    obj->pos = 0;
    for (int i = 0; i < 128; i++) {
        get_bits_raw(obj);
    }
#else
    for (int i = 0; i < 128 - TT800_N; i++) {
        get_bits_raw(obj);
    }
#endif
    int is_ok = 1;
    for (int i = 0; i < 16; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(obj);
        intf->printf("%8.8lX %8.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT32_PRNG(TT800_NAME, run_self_test)

#undef TT800_N
#undef TT800_M
#undef TT800_USE_BUF
#undef TT800_NAME
