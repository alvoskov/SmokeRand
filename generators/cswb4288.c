/**
 * @file cswb4288.c
 * @brief CSWB4288 generator by G. Marsaglia.
 * @details It is a complementary subtract-with-borrow generator based on
 * the next recurrent formula:
 * \f[
 * x_n = (2^{32} - 1) - x_{n-4288} - x_{n-4160} - c_{n-1} \mod 2^{32}
 * \f]
 * \f[
 * c_n = \begin{cases}
 * 1 & \textrm{ if } x_{n-4288} - x_{n-4160} - c_{n-1} < 0
 * 0 & \textrm{ if } x_{n-4288} - x_{n-4160} - c_{n-1} \ge 0
 * \end{cases}
 * \f]
 * It is called SWB-II flavour in the work by Tezuka et al. [2] and is based
 * on the next prime:
 * \f[
 * p = 2^{137210} - 2^{133114} + 1 = b^{4288} - b^{4160} + 1 = B^{2144} - B^{2080} + 1.
 * \f]
 * where \f$ b=2^32 \f$ and \f$ B = 2^{64} \f$.
 *
 * It passes the `express` battery but fails the `gap16_count0` test from
 * `brief`, `default` and `full` batteries (this test is taken from gjrand).
 * In the `full` battery it also fails 2-dimensional birthday spacings tests.
 * Also fails PractRand 0.94.
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
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief CSWB4288 state.
 */
typedef struct {
    uint32_t q[4288];
    int c;
    int ind;
} Cswb4288State;

static inline uint64_t get_bits_raw(void *state)
{
    Cswb4288State *obj = state;
    if (obj->ind < 4288) {
        return obj->q[obj->ind++];
    } else {
        for (int i = 0; i < 4160; i++) {
            uint32_t t = obj->q[i];
            uint32_t h = obj->q[i + 128] + obj->c;
            obj->c = t < h;
            obj->q[i] = h - t - 1;
        }
        for (int i = 4160; i < 4288; i++) {
            uint32_t t = obj->q[i];
            uint32_t h = obj->q[i - 4160] + obj->c;
            obj->c = t < h;
            obj->q[i] = h - t - 1;
        }
        obj->ind = 1;
        return obj->q[0];
    }
}

static void Cswb4288State_init(Cswb4288State *obj, uint32_t xcng, uint32_t xs)
{
    for (int i = 0; i < 4288; i++) {
        xcng = 69069u * xcng + 123u;
        xs ^= (xs << 13);
        xs ^= (xs >> 17);
        xs ^= (xs << 5);
        obj->q[i] = xcng + xs;
    }
    obj->c = 0;
    obj->ind = 4287;
}


static void *create(const CallerAPI *intf)
{
    Cswb4288State *obj = intf->malloc(sizeof(Cswb4288State));
    uint64_t seed = intf->get_seed64();
    Cswb4288State_init(obj, seed >> 32, (seed & 0xFFFFFFFF) | 1);
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    uint32_t x, x_ref = 836315212;
    Cswb4288State *obj = intf->malloc(sizeof(Cswb4288State));
    Cswb4288State_init(obj, 262436069, 532456711); 
    for (unsigned long i = 0; i < 1000000000; i++) {
        x = (uint32_t) get_bits_raw(obj);
    }
    intf->printf("x = %22u; x_ref = %22u\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}


MAKE_UINT32_PRNG("Cswb4288", run_self_test)

