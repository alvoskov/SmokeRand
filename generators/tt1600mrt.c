/**
 * @file tt1600mrt.c
 * @brief TT1600mrt is scrambled version of T1600, a twisted GFSR generator
 * (essentially LFSR), its period is \f$ 2^{1600} - 1 \f$.
 * @details It has a maximal period for LFSR, but the original articles [1,2]
 * don't suggest any tempering. So some custom non-linear bijective tampering
 * was added.
 *
 * The LFSR core of TT1600mrt can be represented as the next recurrent formula:
 *
 * \f[
 * x_{i+n} = x_{i+m} \oplus x_{i} A
 * \f]
 *
 * where x is the the bit ROW vector and A is the bit matrix, \f$\oplus\f$
 * is bitwise XOR. The entire structure strongly resembles xorshift128
 * or xorshift160.
 *
 * Note: if you want to verify the period of TT1600mrt then undefine the
 * `TT1600_USE_BUF` macro: it will enable the slower `NOBUF` version that
 * doesn't use `pos` variable. The verification by means of the `lfsr`
 * battery was successful.
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
 * T1600 PRNG was designed by Matsumoto M. and Kurita Y.
 *
 * Reentrant C99 implementation for SmokeRand and addition of scrambler:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define TT1600_N 25
#define TT1600_M 3
#define TT1600_USE_BUF

#ifdef TT1600_USE_BUF
#define TT1600_NAME "TT1600mrt"
#else
#define TT1600_NAME "TT1600mrt_NOBUF"
#endif

typedef struct {
    uint64_t x[TT1600_N];
#ifdef TT1600_USE_BUF
    int pos;
#endif
} TT1600State;


static inline uint64_t tt1600_mat(uint64_t x)
{
    return ((x & 0x1) == 0) ? (x >> 1) : ((x >> 1) ^ 0xB380C13AA838387E); // a
}

/**
 * @brief Initialize the TT1600 example using a 64-bit seed.
 * @details Expands a 64-bit seed using the scrambled upper 32 bits of
 * 64-bit Klimov-Shamir "crazy" T-function TF0. It allows to avoid zeroland
 * and low quality output at the beginning of the output sequence. The entire
 * procedure resembles an initialization of MT19937 but uses a nonlinear
 * transformation that has a proven full period.
 *
 * It also uses a simple 64-bit LFSR (xorrot64) to decorrelate seeds with
 * differences only in higher bits (remember about T-function)
 */
static void TT1600State_init(TT1600State *obj, uint64_t seed)
{
    expand_seed64_to_u64(obj->x, TT1600_N, seed);
#ifdef TT1600_USE_BUF
    obj->pos = TT1600_N;
#endif
}


static inline uint64_t TT1600State_next(TT1600State *obj)
{
#ifdef TT1600_USE_BUF
    uint64_t *x = obj->x;
    // Refresh the generator state (generate N words)
    if (obj->pos == TT1600_N) {
        for (int i = 0; i < TT1600_N - TT1600_M; i++) {
            x[i] = x[i + TT1600_M] ^ tt1600_mat(x[i]);
        }
        for (int i = TT1600_N - TT1600_M; i < TT1600_N; i++) {
            x[i] = x[i + (TT1600_M - TT1600_N)] ^ tt1600_mat(x[i]);
        }
        obj->pos = 0;
    }
    uint64_t u = x[obj->pos++];
#else
    uint64_t u = obj->x[TT1600_M] ^ tt1600_mat(obj->x[0]);
    for (int i = 0; i < TT1600_N - 1; i++) {
        obj->x[i] = obj->x[i + 1];
    }
    obj->x[TT1600_N - 1] = u;
#endif
    // Tempering/scrambler/output function
    u = rotl64(6906969069U * u, 11);
    u += (u * u | 0x40000005);
    return u;
}


static inline uint64_t get_bits_raw(TT1600State *obj)
{
    return TT1600State_next(obj);
}


static void *create(const CallerAPI *intf)
{
    TT1600State *obj = intf->malloc(sizeof(TT1600State));
    TT1600State_init(obj, intf->get_seed64());
    return obj;
}

/**
 * @brief The internal self-test that is required for comparison between
 * two different modifications of TT1600 (with and without bufferization)
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[16] = {
        0x36BB6BC3368D4732, 0xE271985680C989DC,
        0xF5879F7F6E32A6C7, 0x30320D0845020D06,
        0xBE98E86C37479DDC, 0x4D55B1418F5D2236,
        0xF241192E279AC862, 0x557E730977695A22,
        0x9FE26D00E59DF131, 0xB1852273AA164CC4,
        0x21CFB4A91887AF90, 0xC3D80C47CCD8C5AC,
        0x4BD06C9FE558F60F, 0x840DBAA73B2EB0E1,
        0xA27C05D77FF69EBB, 0xA1735FE7DFC6F8F2
    };
    TT1600State *obj = create(intf);
    TT1600State_init(obj, 0xDEADBEEFCAFEBABE);
    for (int i = 0; i < 128; i++) {
        get_bits_raw(obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 16; i++) {
        const uint64_t u = get_bits_raw(obj);
        intf->printf("%16.16llX %16.16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG(TT1600_NAME, run_self_test)

#undef TT1600_N
#undef TT1600_M
#undef TT1600_USE_BUF
#undef TT1600_NAME
