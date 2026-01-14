/**
 * @file zibri64ex.c
 * @brief Zibri64ex is a modification of Zibri128 chaotic generator
 * made by A.L. Voskov.
 * @details It still fails the hamming_distr test at large samples:
 *
 *
 *    ----- Test 1 of 1 (hamming_distr)
 *    Hamming weights distribution test (histogram)
 *      Sample size, values:     137438953472 (2^37.00 or 10^11.14)
 *      Blocks analysis results
 *            bits |        z          p |    z_xor      p_xor
 *              32 |    0.328      0.371 |   -0.409      0.659
 *              64 |   -1.789      0.963 |    0.434      0.332
 *             128 |    6.804   5.08e-12 |   18.883   7.83e-80
 *             256 |    2.264     0.0118 |    7.626   1.21e-14
 *             512 |   -0.318      0.625 |    3.519   0.000217
 *            1024 |    0.143      0.443 |    4.195   1.36e-05
 *            2048 |    1.303     0.0963 |   -0.067      0.527
 *            4096 |    0.557      0.289 |    0.393      0.347
 *            8192 |    0.811      0.209 |    0.669      0.252
 *           16384 |   -0.626      0.734 |   -0.111      0.544
 *      Final: z =  18.883, p = 7.83e-80
 *
 *    ==================== 'CUSTOM' battery report ====================
 *    Generator name:    Zibri64ex
 *    Output size, bits: 32
 *    Used seed:         _01_+LPlFg5XKqOUyGMd8XHJv6LRbiaRu7rzM/47Y3uqXhI=
 *
 *        # Test name                    xemp              p Interpretation 
 *    ----------------------------------------------------------------------
 *        1 hamming_distr             18.8833       7.83e-80 FAIL           
 *    ----------------------------------------------------------------------
 *    Elapsed time:  00:17:26
 *    Used seed:     _01_+LPlFg5XKqOUyGMd8XHJv6LRbiaRu7rzM/47Y3uqXhI=
 *
 * References:
 *
 * 1. https://github.com/lemire/testingRNG/issues/17
 * 2. https://github.com/Zibri
 * 3. http://www.zibri.org/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t s[2]; ///< Nonlinear part, "mixer".
    uint32_t ctr;  ///< Linear part, "discrete Weyl sequence'.
} Zibri64ExState;


static uint64_t get_bits_raw(Zibri64ExState *obj)
{
    const uint32_t s0 = obj->s[0], s1 = obj->s[1];
    obj->s[0] = rotl32(s0 + s1, 27);
    obj->s[1] = rotl32(s0, 11) + (obj->ctr += 0x9E3779B9);
    return s0 ^ s1;
}

static void *create(const CallerAPI *intf)
{
    Zibri64ExState *obj = intf->malloc(sizeof(Zibri64ExState));
    obj->s[0] = intf->get_seed32();
    obj->s[1] = intf->get_seed32();
    obj->ctr  = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("Zibri64ex", NULL)
