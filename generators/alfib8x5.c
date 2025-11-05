/**
 * @file alfib8x5.c 
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
 * u_i = 3\left(x_i \oplus (x_i \gg 5) \right) \mod 2^8
 * \f]
 *
 * XOR hides low linear complexity of the lowest bits and multiplication ---
 * linear dependencies detected by the matrix rank tests. An initial state
 * is initialized by the custom modification of XABC generator from [2].
 *
 * References:
 *
 * 1. Wu P.-C. Random number generation with primitive pentanomials //
 *    ACM Trans. Model. Comput. Simul. 2001. V. 11. N 4. P.346-351.
 *    https://doi.org/10.1145/508366.508368.
 * 2. Daniel Dunn (aka EternityForest) The XABC Random Number Generator
 *    https://eternityforest.com/doku/doku.php?id=tech:the_xabc_random_number_generator
 * 3. https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=3532b8a75efb3fe454c0d4dd68c1b09309d8288c
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    LF8X5_WARMUP = 32,
    LF8X5_BUFSIZE = 64,
    LF8X5_MASK = 0x3F
};

typedef struct {
    uint8_t x[LF8X5_BUFSIZE];
    uint8_t pos;
} Alfib8State;

static inline uint8_t get_bits8(Alfib8State *obj)
{
    uint8_t *x = obj->x;
    obj->pos++;
    uint8_t u = (uint8_t) ( x[(obj->pos - 61) & LF8X5_MASK]
        + x[(obj->pos - 60) & LF8X5_MASK]
        + x[(obj->pos - 46) & LF8X5_MASK]
        + x[(obj->pos - 45) & LF8X5_MASK] );
    x[obj->pos & LF8X5_MASK] = u;
    // Output scrambler
    // Round 1
    u = (uint8_t) (u ^ (u >> 5)); // Hide low linear complexity of lower bits
    u = (uint8_t) (u + (u << 1)); // To prevent failure of matrix rank tests
    // Round 2
    u = (uint8_t) (u ^ (u >> 6));
    u = (uint8_t) (u + (u << 3));
    return u;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = get_bits8(state);
    }
    return out.u32;    
}

static void Alfib8State_init(Alfib8State *obj, uint32_t seed)
{    
    uint8_t x = (uint8_t) (seed & 0xFF);
    uint8_t a = (uint8_t) ((seed >> 8) & 0xFF);
    uint8_t b = (uint8_t) ((seed >> 16) & 0xFF);
    uint8_t c = (uint8_t) ((seed >> 24) & 0xFF);
    for (int i = 0; i < LF8X5_WARMUP + LF8X5_BUFSIZE; i++) {
        a ^= (uint8_t) (c ^ (x += 151));
        b += a;
        c = (uint8_t) ((c + ((b << 7) | (b >> 1))) ^ a);
        if (i >= LF8X5_WARMUP) {
            obj->x[i - LF8X5_WARMUP] = c ^ b;
        }
    }
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Alfib8State *obj = intf->malloc(sizeof(Alfib8State));
    Alfib8State_init(obj, intf->get_seed32());
    return obj;
}

MAKE_UINT32_PRNG("Alfib8x5", NULL)
