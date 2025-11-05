/**
 * @file xkiss8_awc.c
 * @brief XKISS8/AWC is a 8-bit modificaiton of 32-bit KISS algorithm
 * (2007 version) by G. Marsaglia with parameters tuned by A.L. Voskov.
 * Generates bytes, suitable for 8-bit processors without multiplication
 * support and barrel shift (ROR/ROL) support.
 *
 * @details The initial 32-bit version [2] with slightly altered parameters
 * is known as JKISS32 (see the paper by David Jones [4]). The next changes
 * were made to adapt the algorithm to 16-bit CPUs:
 *
 * 1. xorshift32 was replaced to a custom xorshift-style generator [1].
 * 2. AWC (add-with-carry) generator was tuned for 8-bit machines that
 *    support 8-bit addition with carry bit. It is based on the
 *    \f$ m = (2^{8})^3 + (2^{8})^2 - 1 \f$ prime and essentially a LCG
 *    with prime modulus and bad multiplier.
 * 3. Discrete Weyl sequence was made 8-bit.
 * 4. David Jones, UCL Bioinformatics Group. Good Practice in (Pseudo) Random
 *    Number Generation for Bioinformatics Applications
 *    http://www0.cs.ucl.ac.uk/staff/D.Jones/GoodPracticeRNG.pdf
 *
 * NOTE: xorshift from [1] is no longest period possible, bad seed are
 * possible. But it has reasonable statistical quality for its size.
 * We use the seed recommended by the author.
 *
 * Passes `express`, `brief`, `default` and `full` batteries, also passes
 * extended frequency test at least up to 512 GiB (RC4 doesn't).
 *
 * References:
 *
 * 1. Edward Rosten. https://github.com/edrosten/8bit_rng
 * 2. George Marsaglia. Fortran and C: United with a KISS. 2007.
 *    https://groups.google.com/g/comp.lang.fortran/c/5Bi8cFoYwPE
 * 3. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief XKISS8/AWC generator state.
 */
typedef struct {
    uint8_t s[4]; ///< LFSR state.
    uint8_t x[3]; ///< AWC state: values.
    uint8_t x_c;  ///< AWC state: carry.
    uint8_t weyl; ///< Discrete Weyl sequence state.
} Xkiss8AwcState;


static inline uint8_t get_bits8(Xkiss8AwcState *obj)
{
    // LFSR (xorshift-style) part
    uint8_t tx = (uint8_t) (obj->s[0] ^ (obj->s[0] << 4));
    obj->s[0] = obj->s[1];
    obj->s[1] = obj->s[2];
    obj->s[2] = obj->s[3];
    obj->s[3] = (uint8_t) (obj->s[2] ^ tx ^ (obj->s[2] >> 1) ^ (tx << 1));
    // AWC part
    // b**3 + b**2 + 1 => x_{n} = x_{n-3} + b_{n-2} + c
    uint16_t t = (uint16_t) (
        (uint16_t) obj->x[0] +
        (uint16_t) obj->x[1] +
        (uint16_t) obj->x_c
    );
    uint8_t u = (uint8_t) (t & 0xFF);
    obj->x[0] = obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = u;
    obj->x_c = (uint8_t) (t >> 8);
    // Weyl sequence part
    obj->weyl += 151u;
    // Generate output
    return (uint8_t) (obj->s[0] + u + obj->weyl);
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


static void *create(const CallerAPI *intf)
{
    Xkiss8AwcState *obj = intf->malloc(sizeof(Xkiss8AwcState));
    uint64_t s = intf->get_seed64();
    // AWC initialization
    obj->x[0] = s & 0xFF;
    obj->x[1] = (s >> 8) & 0xFF;
    obj->x[2] = (s >> 16) & 0xFF;
    if (obj->x[0] == 0 && obj->x[1] == 0 && obj->x[2] == 0) {
        obj->x_c = 1;
    } else {
        obj->x_c = 0;
    }
    // Weyl sequence initialization
    obj->weyl = (s >> 24) & 0xFF;
    // LFSR initialization: beware of bad seeds
    obj->s[0] = 0;
    obj->s[1] = 0;
    obj->s[2] = 0;
    obj->s[3] = 1;
    // Generator warmup
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("XKISS8/AWC", NULL)
