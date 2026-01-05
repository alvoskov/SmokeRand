/**
 * @file efiix64x48.c
 * @brief efiix64x48 pseudorandom number generator suggested by
 * Chris Doty-Humphrey the author of PractRand test suite.
 * @details The author claims that efiix is cryptographically secure but
 * the generator cryptoanalysis was never published (SO IT MUSTN'T BE
 * USED AS A CSPRNG WITHOUT CRYPTOANALYSIS! Empirical tests are not enough!)
 *
 * The minimal period of efiix64x48 is 2^64 because of the counter in its
 * state. It shows a good quality in empirical tests and can be used in
 * numerical simulations. Multithreaded use requires further exploration.
 *
 * @copyright efiix64x48 algorithm was developed by Chris Doty-Humphrey,
 * the author of PractRand test suite. This file is based on its public domain
 * implementation from PractRand 0.94 (https://pracrand.sourceforge.net/).
 *
 * - https://groups.google.com/g/sci.crypt.random-numbers/c/55AFQvcsaoU
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    ITERATION_SIZE_L2 = 5,
    ITERATION_SIZE = 1 << ITERATION_SIZE_L2,
    INDIRECTION_SIZE_L2 = 4,
    INDIRECTION_SIZE = 1 << INDIRECTION_SIZE_L2
};


typedef struct {
    uint64_t indirection_table[INDIRECTION_SIZE];
    uint64_t iteration_table[ITERATION_SIZE];
    uint64_t i;
    uint64_t a;
    uint64_t b;
    uint64_t c;
} Efiix64x48State;


static inline uint64_t get_bits_raw(Efiix64x48State *obj)
{
    const uint64_t iterated = obj->iteration_table  [obj->i % ITERATION_SIZE];
    const uint64_t indirect = obj->indirection_table[obj->c % INDIRECTION_SIZE];
    obj->indirection_table[obj->c % INDIRECTION_SIZE] = iterated + obj->a;
    obj->iteration_table  [obj->i % ITERATION_SIZE  ] = indirect;
    const uint64_t old = obj->a ^ obj->b;
    obj->a = obj->b + obj->i++;
    obj->b = obj->c + indirect;
    obj->c = old + rotl64(obj->c, 25);
    return obj->b ^ iterated;
    
}


static void *create(const CallerAPI *intf)
{
    Efiix64x48State *obj = intf->malloc(sizeof(Efiix64x48State));
	obj->a = intf->get_seed32();
	obj->b = intf->get_seed32();
	obj->c = intf->get_seed32();
	obj->i = intf->get_seed32();
	// Number of possible seeded states is kept much smaller than the number of
    // valid states. In order to make it extremely unlikely that any bad seeds
    // exist as opposed to how it would be otherwise, in which finding a bad
    // seed would be *extremely* difficult, but a few would likely exist
    for (int x = 0; x < ITERATION_SIZE; x++) {
        if (!(x & 3)) {
            obj->iteration_table[x] = intf->get_seed32();
        } else {
            obj->iteration_table[x] = obj->iteration_table[x & ~3];
        }
    }
    for (int x = 0; x < INDIRECTION_SIZE; x++) {
        obj->indirection_table[x] = intf->get_seed32();
    }
    // PRNG warmup to improve the quality of the first values
    // in the output.
    for (int x = 0; x < ITERATION_SIZE; x++) {
        (void) get_bits_raw(obj);
    }
    return obj;    
}


MAKE_UINT64_PRNG("efiix64x48", NULL)
