/**
 * @file lrnd64_shared.c
 * @brief A simple LFSR generator with 1024 bits of state and based on
 * 64-bit arithmetics. Fails linear complexity and matrix rank tests.
 * @details It is based on the next recurrent formula:
 *
 * \f[
 * b_{j+1024} = b_{j+512} + b_{j+128} + b_{j+8} + b_{j+1}
 * \f]
 *
 * where b are bits and + is addition modulo 2 (i.e. XOR). This optimized
 * implementation works with 64-bit chunks and a circular buffer.
 *
 * References:
 *
 * 1. M.V. Iakobovski, M.A. Kornilina, M.N. Voroniuk. The Scalable GPU-based
 *    Parallel Algorithm for Uniform Pseudorandom Number Generation", in
 *    "Proceedings of the Second International Conference on Parallel,
 *    Distributed, Grid and Cloud Computing for Engineering", Civil-Comp Press,
 *    Stirlingshire, UK, Paper 23, 2011. http://dx.doi.org/10.4203/ccp.95.23
 * 2. М.Н. Воронюк, М.В. Якобовский. Адаптация алгоритмов моделирования
 *    динамических процессов фильтрации в перколяционных решетках для
 *    графических ускорителей // Математическое моделирование. 2012.
 *    Т. 24. N 12. С. 78--85. http://mi.mathnet.ru/mm3227
 * 3. https://itprojects.narfu.ru/grid/materials2015/Yacobovskii.pdf
 *
 * @copyright The LRND64 algorithm is suggested by M.V. Iakobovski,
 * M.A. Kornilina and M.N. Voroniuk.
 *
 * The optimized reentrant implementation for SmokeRand:
 *
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LRnd64 PRNG state.
 */
typedef struct {
    int w_pos[4];
    uint64_t w[16];
} LRnd64State;

/**
 * @brief Creates LFSR example with taking into account
 * limitations on seeds (initial state).
 */
void *create(const CallerAPI *intf)
{
    LRnd64State *obj = intf->malloc(sizeof(LRnd64State));
    for (int i = 0; i < 16; i++) {
        do {
            obj->w[i] = intf->get_seed64();
        } while (obj->w[i] == 0);
    }
    obj->w_pos[0] = 0;
    obj->w_pos[1] = 1;
    obj->w_pos[2] = 2;
    obj->w_pos[3] = 8;
    return obj;
}

static inline uint64_t get_bits_raw(void *state)
{
    LRnd64State *obj = state;
    uint64_t w0 = obj->w[obj->w_pos[0]];
    uint64_t w1 = obj->w[obj->w_pos[1]];
    uint64_t w2 = obj->w[obj->w_pos[2]];
    uint64_t w8 = obj->w[obj->w_pos[3]];
    // b_{j+1024} = b_{j+512} + b_{j+128} + b_{j+8} + b_{j+1}
    uint64_t w16 = w8 ^ w2 ^ ((w0 >> 8) ^ (w1 << 56)) ^ ((w0 >> 1) ^ (w1 << 63));
    obj->w[obj->w_pos[0]] = w16;
    for (int i = 0; i < 4; i++) {
        obj->w_pos[i] = (obj->w_pos[i] + 1) & 0xF;
    }
    return w16;
}


MAKE_UINT64_PRNG("LRND64", NULL)
