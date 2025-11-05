/**
 * @brief RANMAR algorithm for double precision.
 * @details Passes SmallCrush and Crush. But fails BigCrush
 * if the original c constant is used in "Weyl sequence":
 *
 *     59  WeightDistrib, r = 0           2.6e-13
 *
 * The new c constant allows this PRNG to pass BigCrush:
 * it is very close to \f$ (\sqrt{5} - 1) / 2 \f$ but differs in
 * the last digits to make mantissa odd. It is very similar to
 * "Weyl sequence" in SplitMix.
 *
 * RANMAR fails PractRand:
 * - Original constant: passes at 256 GiB, fails at 512 GiB
 *
 *
 * References:
 *
 * 1. George Marsaglia, Wai Wan Tsang. The 64-bit universal RNG //
 *    // Statistics & Probability Letters. 2004. V. 66. N 2. P.183-187.
 *    https://doi.org/10.1016/j.spl.2003.11.001.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * The KISS99 algorithm is developed by George Marsaglia.
 *
 * All rights reserved.
 *
 * This software is provided under the Apache 2 License.
 */
#include "smokerand/cinterface.h"
#include <limits.h>


// Doesn't fail: hamming_ot         test=hamming_ot      mode=bytes_low1      nbytes=400000000000 # 2^30
#define LFIB_A 98
#define LFIB_B 33


// Failure at:
// hamming_ot         test=hamming_ot      mode=bytes_low1      nbytes=50000000000 # 2^30
//#define LFIB_A 63
//#define LFIB_B 31


SHARED_ENTRYPOINT_CODE


//static const double c = 5566755282872655.0 / 9007199254740992.0; /**< shift */
static const double c = 362436069876.0 / 9007199254740992.0; /**< original shift */


static inline double amb_mod_r(double a, double b)
{
    static const double r = 9007199254740881.0 / 9007199254740992.0; /**< base (prime) */
    double x = a - b;
    return (x >= 0.0) ? x : (x + r);
}

typedef struct {
    double U[LFIB_A + 1];
    double z;
    int i;
    int j;
} LFibFloat;

static inline uint64_t get_bits_raw(void *state)
{
    LFibFloat *obj = (LFibFloat *) state;
    static const double M_2_POW_32 = 4294967296.0;
    // Subtractive Lagged Fibbonaci part
    double x = obj->U[obj->i] - obj->U[obj->j], out;
    if (x < 0.0) x += 1.0;
    obj->U[obj->i] = x;
    if(--obj->i == 0) obj->i = LFIB_A;
	if(--obj->j == 0) obj->j = LFIB_A;
    // Weyl sequence part
    obj->z = amb_mod_r(obj->z, c);
    // Combine two generators
    x -= obj->z;
    out = (x < 0.0) ? (x + 1.0) : x;
    return (uint32_t) (out * M_2_POW_32);
}

static void *create(const CallerAPI *intf)
{
    LFibFloat *obj = intf->malloc(sizeof(LFibFloat));
    uint64_t seed = intf->get_seed64();
    uint32_t x = (uint32_t) (seed & 0xFFFFFFFF);
    uint32_t y = (uint32_t) (seed >> 32);
    for (int i = 1; i <= LFIB_A; i++) {
        double s = 0.0, t = 0.5;
        for (int j = 1; j < 54; j++) {
            x = (6969*x) % 65543;
            y = (8888*y) % 65579; // Original work: y = (8888*x) % 65579;
            if (((x ^ y) & 32) > 0)
                s += t;
            t *= 0.5;
        }
        obj->U[i] = s;
    }
    obj->z = 0;
/*
    for (size_t i = 1; i <= LFIB_A; i++) {
        obj->U[i] = intf->get_seed64() * 5.421010862427522e-20;
    }
*/
    obj->i = LFIB_A; obj->j = LFIB_B;
    return (void *) obj;
}

MAKE_UINT32_PRNG("LFib_RANMAR", NULL)
