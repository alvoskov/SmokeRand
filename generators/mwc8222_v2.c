/**
 * @file cmwc256.c
 * @brief MWC256 (also known as MWC8222) PRNG implementation.
 * @details The algorithm is developed by G.Marsaglia:
 *
 * - https://groups.google.com/g/sci.math/c/k3kVM8KwR-s/m/jxPdZl8XWZkJ
 * - George Marsaglia. Random Number Generators // Journal of Modern Applied
 *   Statistical Methods. 2003. V. 2. N 1. P. 2-13.
 *   https://doi.org/10.22237/jmasm/1051747320
 *
 * This PRNG uses the \f$ b = 2^{32} - 1 \f$ base and the next bithack
 * to implement the carry with such base:
 *
 * \f[
 * t = H\cdot 2^{32} + L = H\cdot b + \underbrace{H + L}_{=S} =
 * H\cdot b + \left(S_{hi} \cdot 2^{32} + S_{lo} \right) =
 * H\cdot b + \left(S_{hi} b + S_{hi} + S_{lo} \right)
 * \f]
 * we should also take into account that \f$S_{hi}\f$ is either 0 or 1.
 * It totally explains mysterous `two ++es` in the `if` branch.
 *
 * The period can be checked by the next Python 3.x script:
 *
 *    import sympy
 *    a, b = 1540315826, 2**32 - 1
 *    m = a*b**256 - 1
 *    print("Is prime: ", sympy.isprime(m))
 *    o = sympy.n_order(b, m)
 *    print("ord: ", o)
 *    print("ord(m) / m: ", o / m)
 *    print(f"{(o - m) / m:.15g}")
 *    print(o - m)
 *
 *
 * @copyright Based on scientific publication by G.Marsaglia.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t Q[256];
    uint32_t c;
    uint8_t pos;
} Mwc8222Ver2State;

static inline uint64_t get_bits_raw(Mwc8222Ver2State *obj)
{
    const uint64_t a = 1540315826ULL;
    const uint64_t t = a * obj->Q[++obj->pos] + obj->c;
    obj->c = (uint32_t) (t >> 32);
    uint32_t x = (uint32_t) (t + obj->c);
    if (x < obj->c) {
        x++;
        obj->c++;
    }
    return obj->Q[obj->pos] = x;
}

static void *create(const CallerAPI *intf)
{
    Mwc8222Ver2State *obj = intf->malloc(sizeof(Mwc8222Ver2State));
    uint64_t seed = intf->get_seed64();
    for (size_t i = 0; i < 256; i++) {
        do {
            seed += seed * seed | 0x40000005;        
            obj->Q[i] = (uint32_t) (seed >> 32);
        } while (obj->Q[i] == 0xFFFFFFFFU);
    }
    obj->c = 123U;
    obj->pos = (uint8_t) 255U;
    return obj;
}

MAKE_UINT32_PRNG("MWC8222_ver2", NULL)
