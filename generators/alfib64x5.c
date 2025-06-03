/**
 * @file alfib64x5.c 
 * @brief A 4-tap additive lagged Fibonacci generator with output scrambler;
 * works only with bytes, doesn't use multiplication, may be used for 8-bit CPUs.
 * @details The next recurrent formula from [1] is used:
 *
 * \f[
 * x_{i} = x_{i - 61} + x_{i - 60} + x_{i - 46} + x_{i - 45} \mod 2^8
 * \f]
 *
 * The next output function is used:
 * 
 * \f[
 * t_i = a\left(x_i \oplus (x_i \gg 52) \right) \mod 2^64
 * \f]
 *
 * \f[
 * u_i = a\left(x_i \oplus (x_i \gg 37) \right) \mod 2^64
 * \f]
 *
 * XOR hides low linear complexity of the lowest bits and multiplication ---
 * linear dependencies detected by the matrix rank tests. An initial state
 * is initialized by the custom modification of XABC generator from [2].
 * The \f$ a \f$ multiplier is taken from xorshift64*.
 *
 * The second round is needed to prevent failures of `hamming_ot_low1` test
 * on a very large samples.
 *
 * The PRNG is initialized by XABC64: a modification of chaotic XABC generator
 * by Daniel Dunn [3].
 *
 * References:
 *
 * 1. Wu P.-C. Random number generation with primitive pentanomials //
 *    ACM Trans. Model. Comput. Simul. 2001. V. 11. N 4. P.346-351.
 *    https://doi.org/10.1145/508366.508368.
 * 2. https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=3532b8a75efb3fe454c0d4dd68c1b09309d8288c
 * 3. Daniel Dunn (aka EternityForest) The XABC Random Number Generator
 *    https://eternityforest.com/doku/doku.php?id=tech:the_xabc_random_number_generator
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    LF64X5_WARMUP = 32,
    LF64X5_BUFSIZE = 64,
    LF64X5_MASK = 0x3F
};

typedef struct {
    uint64_t x[LF64X5_BUFSIZE];
    uint8_t pos;
} Alfib64x5State;

static inline uint64_t get_bits_raw(void *state)
{
    Alfib64x5State *obj = state;
    uint64_t a = 2685821657736338717ULL;
    uint64_t *x = obj->x;
    obj->pos++;
    uint64_t u = x[(obj->pos - 61) & LF64X5_MASK]
        + x[(obj->pos - 60) & LF64X5_MASK]
        + x[(obj->pos - 46) & LF64X5_MASK]
        + x[(obj->pos - 45) & LF64X5_MASK];
    x[obj->pos & LF64X5_MASK] = u;
    // Output scrambler
    // Round 1: hides low inear complexity issues
    u = u ^ (u >> 52); // Hide low linear complexity of lower bits
    u = a * u;         // To prevent failure of matrix rank tests
    // Round 2: removes Hamming weights correlations in the lower bits
    u = u ^ (u >> 37);
    u = a * u;
    return u;
}

/**
 * @brief Initializes the generator by means of the XABC64 chaotic PRNG.
 */
static void Alfib64x5State_init(Alfib64x5State *obj, uint64_t seed)
{    
    uint64_t x = 0;
    uint64_t a = seed;
    uint64_t b = ~seed;
    uint64_t c = 0xDEADBEEFDEADBEEF;
    for (int i = 0; i < LF64X5_WARMUP + LF64X5_BUFSIZE; i++) {
        a ^= c ^ (x += 0x9E3779B97F4A7C15);
        b += a;
        c = (c + rotl64(b, 12)) ^ a;
        if (i >= LF64X5_WARMUP) {
            obj->x[i - LF64X5_WARMUP] = c ^ b;
        }
    }
    obj->x[0] |= 1; obj->x[1] &= 0;
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Alfib64x5State *obj = intf->malloc(sizeof(Alfib64x5State));
    Alfib64x5State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT64_PRNG("Alfib64x5", NULL)
