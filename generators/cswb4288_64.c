/**
 * @file cswb4288_64.c
 * @brief CSWB4288 generator by G. Marsaglia: 64-bit version.
 * @details It is a complementary subtract-with-borrow generator based on
 * the next recurrent formula:
 * \f[
 * x_n = (2^{64} - 1) - x_{n-2144} - x_{n-2080} - c_{n-1} \mod 2^{64}
 * \f]
 * \f[
 * c_n = \begin{cases}
 * 1 & \textrm{ if } x_{n-2144} - x_{n-2080} - c_{n-1} < 0
 * 0 & \textrm{ if } x_{n-2144} - x_{n-2080} - c_{n-1} \ge 0
 * \end{cases}
 * \f]
 * It is based on the next prime:
 * \f[
 * p = 2^{137216} - 2^{133120} + 1 = b^{4288} - b^{4160} + 1 = B^{2144} - B^{2080} + 1.
 * \f]
 * where \f$ b=2^32 \f$ and \f$ B = 2^{64} \f$.
 *
 * The behaviour is similar to the original 32-bit version of CSWB4288. It fails
 * `gap16_count0`, `bspace64_1d`, `bspace32_2d`, `bspace32_2d_high` tests that
 * still makes it too flawed for scientific/engineering applications.
 *
 * References:
 * 
 * 1. G. Marsaglia. An interesting new RNG.
 *    https://www.thecodingforums.com/threads/an-interesting-new-rng.727086/
 * 2. Shu Tezuka, Pierre L'Ecuyer, Raymond Couture. On the lattice structure
 *    of the add-with-carry and subtract-with-borrow random number generators //
 *    // ACM Transactions on Modeling and Computer Simulation (TOMACS). 1993.
 *    V. 3. N 4. P. 315 - 331. https://doi.org/10.1145/159737.159749
 * 3. George Marsaglia, Arif Zaman. A New Class of Random Number Generators //
 *    Ann. Appl. Probab. 1991. V. 1. N.3. P. 462-480
 *    https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright The CSWB4288 algorithm was developed by G. Marsaglia.
 * 64-bit version and reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

#define CSWB64_LAGR 2144
#define CSWB64_LAGS 2080

/**
 * @brief CSWB4288/64 state.
 */
typedef struct {
    uint64_t q[CSWB64_LAGR];
    unsigned int c;
    int ind;
} Cswb4288x64State;


static inline uint64_t get_bits_raw(void *state)
{
    Cswb4288x64State *obj = state;
    if (obj->ind < CSWB64_LAGR) {
        return obj->q[obj->ind++];
    } else {
        for (int i = 0; i < CSWB64_LAGS; i++) {
            uint64_t t = obj->q[i];
            uint64_t h = obj->q[i + (CSWB64_LAGR - CSWB64_LAGS)] + obj->c;
            obj->c = t < h;
            obj->q[i] = h - t - 1;
        }
        for (int i = CSWB64_LAGS; i < CSWB64_LAGR; i++) {
            uint64_t t = obj->q[i];
            uint64_t h = obj->q[i - CSWB64_LAGS] + obj->c;
            obj->c = t < h;
            obj->q[i] = h - t - 1;
        }
        obj->ind = 1;
        return obj->q[0];
    }
}

/**
 * @brief Initialize the generator state using SuperDuper64 PRNG.
 * The original 32-bit version of CSWB4288 uses a later version
 * of SuperDuper32.
 */
static void Cswb4288x64State_init(Cswb4288x64State *obj, uint64_t xcng, uint64_t xs)
{
    for (int i = 0; i < CSWB64_LAGR; i++) {
        xcng = 6906969069ull * xcng + 1234567ull;
        xs ^= (xs << 13);
        xs ^= (xs >> 17);
        xs ^= (xs << 43);
        obj->q[i] = xcng + xs;
    }
    obj->c = 0;
    obj->ind = CSWB64_LAGR;
}


static void *create(const CallerAPI *intf)
{
    Cswb4288x64State *obj = intf->malloc(sizeof(Cswb4288x64State));
    uint64_t seed1 = intf->get_seed64();
    uint64_t seed2 = intf->get_seed64();
    Cswb4288x64State_init(obj, seed1, seed2);
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    uint64_t x, x_ref = 0x3397364FD667C011;
    Cswb4288x64State *obj = intf->malloc(sizeof(Cswb4288x64State));
    Cswb4288x64State_init(obj, 262436069, 532456711); 
    for (unsigned long i = 0; i < 20000000; i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("x = %20.16" PRIX64 "; x_ref = %20.16" PRIX64 "\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}


MAKE_UINT64_PRNG("Cswb4288/64", run_self_test)
