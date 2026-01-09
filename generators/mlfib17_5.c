/**
 * @file mlfib17_5.c
 * @brief A shared library that implements the multiplicative
 * Lagged Fibbonaci generator \f$ LFib(2^{64}, 17, 5, *) \f$.
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - 17} * X_{n - 5} \mathop{\mathrm{mod}} 2^{64}
 * \f]
 * and returns the higher 32 bits The initial values in the ring buffer
 * are filled by the 64-bit PCG generator. Lower 32 bits of this PRNG
 * have bad quality because of all numbers are odd; that causes biases
 * in lower bits distribution.
 *
 * Passes BigCrush.
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define LFIB_A 17
#define LFIB_B 5

PRNG_CMODULE_PROLOG

/**
 * @brief LFIB(LFIB_A, LFIB_B, *) PRNG state.
 */
typedef struct {
    uint64_t U[LFIB_A + 1]; ///< Ring buffer (only values 1..17 are used)
    int i;
    int j;
} MLFib17_5_State;


static inline uint64_t get_bits_raw(MLFib17_5_State *obj)
{
    const uint64_t x = obj->U[obj->i] * obj->U[obj->j];
    obj->U[obj->i] = x;
    if (--obj->i == 0) obj->i = LFIB_A;
	if (--obj->j == 0) obj->j = LFIB_A;
    return x >> 32;
}


static void *create(const CallerAPI *intf)
{
    MLFib17_5_State *obj = intf->malloc(sizeof(MLFib17_5_State));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (size_t k = 1; k <= LFIB_A; k++) {    
        obj->U[k] = pcg_bits64(&state) | 0x1; // Must be ODD!
    }
    obj->i = LFIB_A; obj->j = LFIB_B;
    return obj;
}

MAKE_UINT32_PRNG("MLFib17_5", NULL)
