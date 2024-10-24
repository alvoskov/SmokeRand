/**
 * @file isaac64_shared.c
 * @brief Implementation of ISAAC64 CSPRNG developed by Bob Jenkins.
 * @details It is an adaptation of the original implementation by Bob Jenkins
 * to the multi-threaded environment and C99 standard. References:
 *
 * 1. https://www.burtleburtle.net/bob/rand/isaacafa.html
 * 2. R.J. Jenkins Jr. ISAAC // Fast Software Encryption. Third International
 *    Workshop Proceedings. Cambridge, UK, Februrary 21-23, 1996. P.41-49.
 *
 * @copyright Based on public domain code by Bob Jenkins (1996).
 *
 * Adaptation for C99 and SmokeRand (data types, interface, internal
 * self-test, slight modification of seeding procedure):
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)

/**
 * @brief ISAAC64 CSPRNG state.
 */
typedef struct {
    uint64_t randrsl[RANDSIZ]; ///< Results
    uint64_t mm[RANDSIZ];      ///< Memory
    uint64_t aa; ///< Accumulator
    uint64_t bb; ///< The previous results
    uint64_t cc; ///< Counter
    size_t pos;  ///< Position in the buffer for one-valued outputs
} Isaac64State;


#define ind(mm,x)  (*(uint64_t *)((uint8_t *)(mm) + ((x) & ((RANDSIZ-1)<<3))))
#define rngstep(mix, a, b, mm, m, m2, r, x) \
{ \
    x = *m;  \
    a = (mix) + *(m2++); \
    *(m++) = y = ind(mm,x) + a + b; \
    *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

#define mix(a, b, c, d, e, f, g, h) \
{ \
    a -= e; f ^= h >> 9;  h += a; \
    b -= f; g ^= a << 9;  a += b; \
    c -= g; h ^= b >> 23; b += c; \
    d -= h; a ^= c << 15; c += d; \
    e -= a; b ^= d >> 14; d += e; \
    f -= b; c ^= e << 20; e += f; \
    g -= c; d ^= f >> 17; f += g; \
    h -= d; e ^= g << 14; g += h; \
}

/**
 * @brief Generate a block of pseudorandom numbers.
 */
void Isaac64State_block(Isaac64State *obj)
{    
    uint64_t *mm = obj->mm;
    uint64_t *m = obj->mm, *r = obj->randrsl;
    uint64_t a = obj->aa, b = obj->bb + (++obj->cc);
    uint64_t x, y, *m2, *mend;

    for (m = mm, mend = m2 = m + (RANDSIZ/2); m < mend;)
    {
        rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x);
        rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x);
        rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x);
        rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x);
    }
    for (m2 = mm; m2 < mend; )
    {
        rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x);
        rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x);
        rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x);
        rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x);
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
    static const uint64_t phi = 0x9e3779b97f4a7c13ULL; // The golden ratio
    uint64_t a, b, c, d, e, f, g, h;
    uint64_t *mm = obj->mm, *r = obj->randrsl;
    obj->aa = obj->bb = obj->cc = 0;
    a = b = c = d = e = f = g = h = phi;
    // Scramble it
    for (size_t i = 0; i < 4; i++) {
        mix(a, b, c, d, e, f, g, h);
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
        a += r[i  ]; b += r[i + 1]; c += r[i + 2]; d += r[i + 3];
        e += r[i+4]; f += r[i + 5]; g += r[i + 6]; h += r[i + 7];
        mix(a, b, c, d, e, f, g, h);
        mm[i]     = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
        mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
    }
    // Do a second pass to make all of the seed affect all of mm
    for (size_t i = 0; i < RANDSIZ; i += 8) {
        a += mm[i  ];   b += mm[i + 1]; c += mm[i + 2]; d += mm[i + 3];
        e += mm[i + 4]; f += mm[i + 5]; g += mm[i + 6]; h += mm[i + 7];
        mix(a, b, c, d, e, f, g, h);
        mm[i]     = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
        mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
    }
    Isaac64State_block(obj); // fill in the first set of results
    obj->pos = RANDSIZ; // prepare to use the first set of results
}


static uint64_t get_bits_raw(void *state)
{
    Isaac64State *obj = state;
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
