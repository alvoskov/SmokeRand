/**
 * @file lrnd64_255.c
 * @brief A simple LFSR generator with 255 bits of state and based on
 * 64-bit arithmetics. Fails linear complexity, matrix rank, gap_inv512
 * and gap16_count0 tests.
 * @details It is based on the next recurrent formula:
 *
 * \f[
 * b_{j+256} = b_{j+32} + b_{j+8} + b_{j+4} + b_{j+1}
 * \f]
 *
 * where b are bits and + is addition modulo 2 (i.e. XOR). This optimized
 * implementation works with 64-bit chunks and a circular buffer. It also can
 * be represented as the next primitive GF(2) polynomial:
 *
 * \f[
 * G(x) = x^{255} + x^{31} + x^{7} + x^{3} + 1
 * \f]
 *
 * The original implementation by M.V. Iakobovskii [3] is rather slow, about
 * 100-200 times slower than hardware AES or AVX2 versions of ChaCha12. Its
 * default API is NOT REENTRANT (however, it is possible to make it reentrant).
 * Because of absence of license file it cannot be included in SmokeRand
 * distribution. However, it doesn't fail the `gap_inv512` test because its
 * output is 32-bit. If our optimized 64-bit implementation will be tested with
 * the `--filter=interleaved32` key - it will also pass this test.
 *
 * References:
 *
 * 1. M.V. Iakobovskii. Parallel algorithm of pseudorandom numbers generation
 *    // Matem. Modelirovanie. 2009. V. 21. N 6. P. 59-68 [in Russian].
 *    М.В. Якобовский. Параллельный алгоритм генерации последовательностей
 *    псевдослучайных чисел // Матем. моделирование. 2009. Т. 21. N 6.
 *    С. 59-68 http://mi.mathnet.ru/mm2844
 * 2. M.V. Iakobovskii. An introduction to parallel methods of problems
 *    solving. Izdatelsvo Moskovskogo Universiteta. 2013 [in Russian].
 *    М.В. Якобовский. Введение в параллельные методы решения задач.
 *    Издательство Московского университета. 2013.
 * 3. https://lira.imamod.ru/projects/FondProgramm/RndLib/lrnd32_v02/
 * 4. M.V. Iakobovski, M.A. Kornilina, M.N. Voroniuk. The Scalable GPU-based
 *    Parallel Algorithm for Uniform Pseudorandom Number Generation", in
 *    "Proceedings of the Second International Conference on Parallel,
 *    Distributed, Grid and Cloud Computing for Engineering", Civil-Comp Press,
 *    Stirlingshire, UK, Paper 23, 2011. http://dx.doi.org/10.4203/ccp.95.23
 * 5. https://itprojects.narfu.ru/grid/materials2015/Yacobovskii.pdf
 *
 * @copyright The LRND algorithm is suggested by M.V. Iakobovskii.
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
 * @brief LRnd64-255 PRNG state.
 */
typedef struct {
    uint64_t w[4];
    int8_t w_pos;
} LRnd64x255State;

/**
 * @brief Creates LFSR example with taking into account
 * limitations on seeds (initial state).
 */
void *create(const CallerAPI *intf)
{
    LRnd64x255State *obj = intf->malloc(sizeof(LRnd64x255State));
    for (int i = 0; i < 4; i++) {
        do {
            obj->w[i] = intf->get_seed64();
        } while (obj->w[i] == 0);
    }
    obj->w_pos = 0;
    return obj;
}

/**
 * @brief Implementation of the LFSR defined by the next recurrent formula:
 * \f[
 * b_{j+256} = b_{j+32} + b_{j+8} + b_{j+4} + b_{j+1}
 * \f]
 */
static inline uint64_t get_bits_raw(void *state)
{
    LRnd64x255State *obj = state;
    int8_t ind = obj->w_pos;
    int8_t ind_next = (ind + 1) & 0x3;
    uint64_t w0 = obj->w[ind], w1 = obj->w[ind_next];
    uint64_t w4 = (w0 >> 1) | (w1 << 63);
    w4 ^= (w0 >> 4) | (w1 << 60);
    w4 ^= (w0 >> 8) | (w1 << 56);
    w4 ^= (w0 >> 32) | (w1 << 32);
    obj->w[ind] = w4;
    obj->w_pos = ind_next;
    return w4;
}


MAKE_UINT64_PRNG("LRND64_255", NULL)
