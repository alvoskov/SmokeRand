/**
 * @file pqrng32.c
 * @brief A nonlinear 32-bit generator with an invertible mapping.
 * @details This PRNG is developed by Karl-Uwe Frank and is based
 * on a nonlinear invertible mapping.
 *
 * It is based on the next formula:
 *
 * \f[
 * x_n = \left(x_{n-1} \oplus r\right) p \mod 2^{32}
 * \f]
 *
 * The author recommends the \f$ r \mod 8 = 5 \f$ and \f$ p \mod 8 = 3 \f$
 * constraints but from [3,4] it can be concluded that the milder constraints
 * \f$ r \mod 2 = 1 \f$ and \f$ p \mod 4 = 3 \f$ are enough.
 *
 * References:
 *
 * 1. http://www.freecx.co.uk/pqRNG/
 * 2. https://groups.google.com/g/sci.crypt.random-numbers/c/55AFQvcsaoU
 * 3. https://www.mi-ras.ru/obs_sem/2005/051117/ana_r.pdf
 * 4. V. S. Anachin. Uniformly distributed sequences of p-adic integers //
 *    Mathematical Notes. 1994. V. 55. P. 109–133.
 *    https://doi.org/10.1007/BF02113290
 *
 * The next empirical test proves the full period of this generator
 * (only for the 32-bit version):
 *
 *    #include <stdio.h>
 *    #include <stdint.h>
 *    int main() {
 *        long long ctr = 0;
 *        static const uint32_t r = 5;//0x517CC1B5; // (mod 8 = 5)
 *        static const uint32_t p = 3;//0x9E3779BB; // (mod 8 = 3)
 *        uint32_t x = 0;
 *        do {
 *            x = (x ^ r) * p; ctr++;
 *            if (ctr % (1ULL << 28) == 0) printf("----\n");
 *        } while (x != 0);
 *        printf("%lld\n", ctr);
 *        return 0;
 *    }
 *
 * @copyright pqrng algorithm was developed by Karl-Uwe Frank and is based
 * on invertible mappings suggested by V. S. Anashin.
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

typedef struct {
    uint32_t x;
} Pqrng32State;


static inline uint64_t get_bits_raw(Pqrng32State *obj)
{
    static const uint32_t r = 0x517CC1B5; // (mod8 = 5)
    static const uint32_t p = 0x9E3779BB; // (mod8 = 3)
    obj->x = (obj->x ^ r) * p;
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Pqrng32State *obj = intf->malloc(sizeof(Pqrng32State));
    obj->x = intf->get_seed32();
    return obj;
}


MAKE_UINT32_PRNG("PQRNG32", NULL)
