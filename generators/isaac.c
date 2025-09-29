/**
 * @file isaac.c
 * @brief Implementation of ISAAC cryptographical PRNG developed by Bob Jenkins.
 * @details It is an adaptation of the original implementation by Bob Jenkins
 * to the multi-threaded environment and C99 standard. References:
 *
 * 1. https://www.burtleburtle.net/bob/rand/isaacafa.html
 * 2. R.J. Jenkins Jr. ISAAC // Fast Software Encryption. Third International
 *    Workshop Proceedings. Cambridge, UK, Februrary 21-23, 1996. P.41-49.
 * 3. J.-P. Aumasson. On the pseudo-random generator ISAAC // Cryptology ePrint
 *    Archive. 2006. Paper 2006/438. https://eprint.iacr.org/2006/438
 * 4. M. Pudovkina. A known plaintext attack on the ISAAC keystream generator.
 *    Cryptology ePrint Archive. 2001. Paper 2001/049.2001.
 *    https://eprint.iacr.org/2001/049
 *
 * NOTE: in this implementation it is seeded by means of PCG64/64 using one
 * 64-bit seed. It is suitable for statistical testing but unacceptable for
 * cryptographic purposes. DON'T USE FOR CRYPTOGRAPHY!
 *
 * @copyright Based on public domain code by Bob Jenkins (1996).
 *
 * Adaptation for C99 and SmokeRand (data types, interface, internal
 * self-test, slight modification of seeding procedure):
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)

/**
 * @brief ISAAC cruptographical PRNG state.
 */
typedef struct {
    uint32_t randrsl[RANDSIZ]; ///< Results
    uint32_t mm[RANDSIZ];      ///< Memory
    uint32_t aa; ///< Accumulator
    uint32_t bb; ///< The previous results
    uint32_t cc; ///< Counter
    size_t pos;  ///< Position in the buffer for one-valued outputs
} IsaacState;


static inline uint32_t ind(uint32_t *mm, uint32_t x)
{
    return mm[x & ((uint32_t) (RANDSIZ - 1))];
}

#define rngstep(mix, a, b, mm, m, m2, r) \
{ \
    uint32_t x, y; \
    x = *m;  \
    a = (a ^ (mix)) + *(m2++); \
    *(m++) = y = ind(mm, x >> 2) + a + b; \
    *(r++) = b = ind(mm, y >> (2 + RANDSIZL)) + x; \
}


static inline void mix(uint32_t *x)
{
    x[0] ^= x[1] << 11; x[3] += x[0]; x[1] += x[2];
    x[1] ^= x[2] >> 2;  x[4] += x[1]; x[2] += x[3];
    x[2] ^= x[3] << 8;  x[5] += x[2]; x[3] += x[4];
    x[3] ^= x[4] >> 16; x[6] += x[3]; x[4] += x[5];
    x[4] ^= x[5] << 10; x[7] += x[4]; x[5] += x[6];
    x[5] ^= x[6] >> 4;  x[0] += x[5]; x[6] += x[7];
    x[6] ^= x[7] << 8;  x[1] += x[6]; x[7] += x[0];
    x[7] ^= x[0] >> 9;  x[2] += x[7]; x[0] += x[1];
}

/**
 * @brief Generate a block of pseudorandom numbers.
 */
void IsaacState_block(IsaacState *obj)
{    
    uint32_t *mm = obj->mm, *m = obj->mm, *m2, *mend;
    uint32_t *r = obj->randrsl;
    uint32_t a = obj->aa, b = obj->bb + (++obj->cc);

    for (m = mm, mend = m2 = m + (RANDSIZ/2); m < mend;) {
        rngstep(a << 13, a, b, mm, m, m2, r);
        rngstep(a >> 6 , a, b, mm, m, m2, r);
        rngstep(a << 2 , a, b, mm, m, m2, r);
        rngstep(a >> 16, a, b, mm, m, m2, r);
    }
    for (m2 = mm; m2 < mend; ) {
        rngstep(a << 13, a, b, mm, m, m2, r);
        rngstep(a >> 6,  a, b, mm, m, m2, r);
        rngstep(a << 2,  a, b, mm, m, m2, r);
        rngstep(a >> 16, a, b, mm, m, m2, r);
    }
    obj->bb = b; obj->aa = a;
}

/**
 * @brief Initialize the PRNG state using the supplied seed.
 * @param obj   State to be initialized.
 * @param seed  64-bit random seed used for intialization.
 */
void IsaacState_init(IsaacState *obj, uint64_t seed)
{
    uint32_t x[8];
    uint32_t *mm = obj->mm, *r = obj->randrsl;
    obj->aa = obj->bb = obj->cc = 0;
    for (size_t i = 0; i < 8; i++) {
        x[i] = 0x9e3779b9; // The golden ratio
    }
    // Scramble it
    for (size_t i = 0; i < 4; i++) {
        mix(x);
    }
    // Fill mm[] array with zeros
    for (size_t i = 0; i < RANDSIZ; i++) {
        mm[i] = 0;
    }
    // Fill randrsl[] with PCG64
    // If seed is 0 -- then fill with zeros.
    if (seed == 0) {    
        for (size_t i = 0; i < RANDSIZ; i++) r[i] = 0;
    } else {
        for (size_t i = 0; i < RANDSIZ; i++) {
            r[i] = (uint32_t) pcg_bits64(&seed);
        }
    }
    // Fill in mm[] with messy stuff
    for (size_t i = 0; i < RANDSIZ; i += 8) { 
        for (size_t j = 0; j < 8; j++) { x[j] += r[i + j]; }
        mix(x);
        for (size_t j = 0; j < 8; j++) { mm[i + j] = x[j]; }
    }
    // Do a second pass to make all of the seed affect all of mm
    for (size_t i = 0; i < RANDSIZ; i += 8) {
        for (size_t j = 0; j < 8; j++) { x[j] += mm[i + j]; }
        mix(x);
        for (size_t j = 0; j < 8; j++) { mm[i + j] = x[j]; }
    }
    IsaacState_block(obj); // fill in the first set of results
    obj->pos = RANDSIZ; // prepare to use the first set of results
}


static uint64_t get_bits_raw(void *state)
{
    IsaacState *obj = state;
    if (obj->pos-- == 0) {
        IsaacState_block(obj);
        obj->pos = RANDSIZ - 1;
    }
    return obj->randrsl[obj->pos];
}


static void *create(const CallerAPI *intf)
{
    IsaacState *obj = intf->malloc(sizeof(IsaacState));
    IsaacState_init(obj, intf->get_seed64());
    return obj;
}

/**
 * @brief The internal self-test that compares the PRNG output with
 * the values obtained from the reference implementation of ISAAC64
 * by Bob Jenkins.
 */
static int run_self_test(const CallerAPI *intf)
{
    // Elements 248-255
    int is_ok = 1;
    uint32_t ref[] = {
        0x9d8d1908, 0x86ba527f, 0xf943f672, 0xef73fbf0,
        0x46d95ca5, 0xc54cd95b, 0x9d855e89, 0x4bb5af29};
    IsaacState *obj = intf->malloc(sizeof(IsaacState));
    IsaacState_init(obj, 0);
    for (int i = 0; i < 2; i++) {
        intf->printf("----- BLOCK RUN %d -----\n", i);
        IsaacState_block(obj);
        for (size_t j = 0; j < RANDSIZ; j++) {
            if (j % 8 == 0) {
                intf->printf("%.2x-%.2x: ", (int) j, (int)(j + 3));
            }
            intf->printf("%.8lx", (unsigned long) obj->randrsl[j]);
            if ( (j & 7) == 7)
                intf->printf("\n");
        }
    }

    for (int i = 0; i < 8; i++) {
        if (obj->randrsl[248 + i] != ref[i]) {
            is_ok = 0;
            break;
        }
    }

    intf->free(obj);
    return is_ok;
}

MAKE_UINT32_PRNG("ISAAC", run_self_test)
