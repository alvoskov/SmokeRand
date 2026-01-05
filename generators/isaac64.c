/**
 * @file isaac64.c
 * @brief Implementation of ISAAC64 cryptographic PRNG developed by Bob Jenkins.
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
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)

/**
 * @brief ISAAC64 cryptographic PRNG state.
 */
typedef struct {
    uint64_t randrsl[RANDSIZ]; ///< Results
    uint64_t mm[RANDSIZ];      ///< Memory
    uint64_t aa; ///< Accumulator
    uint64_t bb; ///< The previous results
    uint64_t cc; ///< Counter
    size_t pos;  ///< Position in the buffer for one-valued outputs
} Isaac64State;


static inline uint64_t ind(uint64_t *mm, uint64_t x)
{
    return mm[x & ((uint64_t) (RANDSIZ - 1))];
}


#define rngstep(mix, a, b, mm, m, m2, r) \
{ \
    uint64_t x, y; \
    x = *m;  \
    a = (mix) + *(m2++); \
    *(m++) = y = ind(mm, x >> 3) + a + b; \
    *(r++) = b = ind(mm, y >> (3 + RANDSIZL)) + x; \
}


static inline void mix(uint64_t *x)
{
    x[0] -= x[4]; x[5] ^= x[7] >> 9;  x[7] += x[0];
    x[1] -= x[5]; x[6] ^= x[0] << 9;  x[0] += x[1];
    x[2] -= x[6]; x[7] ^= x[1] >> 23; x[1] += x[2];
    x[3] -= x[7]; x[0] ^= x[2] << 15; x[2] += x[3];
    x[4] -= x[0]; x[1] ^= x[3] >> 14; x[3] += x[4];
    x[5] -= x[1]; x[2] ^= x[4] << 20; x[4] += x[5];
    x[6] -= x[2]; x[3] ^= x[5] >> 17; x[5] += x[6];
    x[7] -= x[3]; x[4] ^= x[6] << 14; x[6] += x[7];
}

/**
 * @brief Generate a block of pseudorandom numbers.
 */
void Isaac64State_block(Isaac64State *obj)
{    
    uint64_t *mm = obj->mm, *m = obj->mm, *m2, *mend;
    uint64_t *r = obj->randrsl;
    uint64_t a = obj->aa, b = obj->bb + (++obj->cc);

    for (m = mm, mend = m2 = m + (RANDSIZ/2); m < mend;) {
        rngstep(~(a^(a<<21)), a, b, mm, m, m2, r);
        rngstep(  a^(a>>5)  , a, b, mm, m, m2, r);
        rngstep(  a^(a<<12) , a, b, mm, m, m2, r);
        rngstep(  a^(a>>33) , a, b, mm, m, m2, r);
    }
    for (m2 = mm; m2 < mend; ) {
        rngstep(~(a^(a<<21)), a, b, mm, m, m2, r);
        rngstep(  a^(a>>5)  , a, b, mm, m, m2, r);
        rngstep(  a^(a<<12) , a, b, mm, m, m2, r);
        rngstep(  a^(a>>33) , a, b, mm, m, m2, r);
    }
    obj->bb = b; obj->aa = a;
}

/**
 * @brief Initialize the PRNG state using the supplied seed.
 * @param obj   State to be initialized.
 * @param seed  64-bit random seed used for intialization.
 */
void Isaac64State_init(Isaac64State *obj, uint64_t seed)
{
    uint64_t x[8];
    uint64_t *mm = obj->mm, *r = obj->randrsl;
    obj->aa = obj->bb = obj->cc = 0;
    for (size_t i = 0; i < 8; i++) {
        x[i] = 0x9e3779b97f4a7c13ULL; // The golden ratio
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
            r[i] = pcg_bits64(&seed);
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
    Isaac64State_block(obj); // fill in the first set of results
    obj->pos = RANDSIZ; // prepare to use the first set of results
}


static uint64_t get_bits_raw(Isaac64State *obj)
{
    if (obj->pos-- == 0) {
        Isaac64State_block(obj);
        obj->pos = RANDSIZ - 1;
    }
    return obj->randrsl[obj->pos];
}


static void *create(const CallerAPI *intf)
{
    Isaac64State *obj = intf->malloc(sizeof(Isaac64State));
    Isaac64State_init(obj, intf->get_seed64());
    return (void *) obj;
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
    uint64_t ref[] = {0x1bda0492e7e4586eull, 0xd23c8e176d113600ull,
        0x252f59cf0d9f04bbull, 0xb3598080ce64a656ull,
        0x993e1de72d36d310ull, 0xa2853b80f17f58eeull,
        0x1877b51e57a764d5ull, 0x001f837cc7350524ull};

    Isaac64State *obj = intf->malloc(sizeof(Isaac64State));
    Isaac64State_init(obj, 0);
    for (int i = 0; i < 2; i++) {
        intf->printf("----- BLOCK RUN %d -----\n", i);
        Isaac64State_block(obj);
        for (size_t j = 0; j < RANDSIZ; j++) {
            if (j % 4 == 0) {
                intf->printf("%.2x-%.2x: ", (int) j, (int)(j + 3));
            }
            intf->printf("%.8lx%.8lx",
                (uint32_t) (obj->randrsl[j] >> 32),
                (uint32_t) obj->randrsl[j]);
            if ( (j & 3) == 3)
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

MAKE_UINT64_PRNG("ISAAC64", run_self_test)
