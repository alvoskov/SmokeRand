/**
 * @file melg44497.c
 * @brief An implementation of MELG44497-64: a GFSR that resembles Mersenne
 * Twister.
 * @details Test values were obtained from the reference implementation
 * provided by the authors [2]. This module is a simplified version of it.
 * It doesn't include transition matrix but all global variables were removed.
 *
 * References:
 *
 * 1. Hasare S., Kimoto T. Implementing 64-bit Maximally Equidistributed
 *    F2-Linear Generators with Mersenne Prime Period // ACM Trans. Math.
 *    Softw. 2018. V. 44. N 3. ID 30. 11 pages. https://doi.org/10.1145/3159444
 * 2. https://github.com/sharase/melg-64
 *
 * @copyright
 *
 * (c) 2021 Shin Harase (Ritsumeikan University, harase @ fc.ritsumei.ac.jp),
 * Takamitsu Kimoto
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define NN 695 //N-1
#define MM 373
#define MATRIX_A 0x4fa9ca36f293c9a9ULL
#define P 17 //W-r
#define W 64
#define MASKU (0xffffffffffffffffULL << (W-P))
#define MASKL (~MASKU)
#define MAT3NEG(t, v) (v ^ (v << ((t))))
#define MAT3POS(t, v) (v ^ (v >> ((t))))
#define LAG1 95
#define SHIFT1 6
#define MASK1 0x6fbbee29aaefd91ULL
#define LAG1over 600 //NN-LAG1
static uint64_t mag01[2]={0ULL, MATRIX_A};

typedef struct MelgState {
    uint64_t lung;
    uint64_t melg[NN];
    int pos;
    uint64_t (*function_p)(struct MelgState *obj);
} MelgState;


static uint64_t case_1(MelgState *obj);
static uint64_t case_2(MelgState *obj);
static uint64_t case_3(MelgState *obj);
static uint64_t case_4(MelgState *obj);

/* initializes melg[NN] and lung with a seed */
void MelgState_init(MelgState *obj, uint64_t seed)
{
    for (unsigned int pos = 0; pos < NN; pos++) {
        obj->melg[pos] = seed;
        seed = (6364136223846793005ULL * (seed ^ (seed >> 62)) + pos + 1);
    }
    obj->lung = seed;
    obj->pos = 0;
    obj->function_p = case_1;
}


static inline uint64_t case_generic(MelgState *obj,
    int sh1, int sh2, int sh3,
    uint64_t (*new_function_p)(MelgState *obj))
{
    uint64_t *st = obj->melg;
    uint64_t x = (st[obj->pos] & MASKU) | (st[obj->pos+1] & MASKL);
    obj->lung = (x >> 1) ^ mag01[(int)(x & 1ULL)] ^ st[obj->pos + sh1] ^ MAT3NEG(37, obj->lung);
    st[obj->pos] = x ^ MAT3POS(14, obj->lung);
    x = st[obj->pos] ^ (st[obj->pos] << SHIFT1);
    x = x ^ (st[obj->pos + sh2] & MASK1);
    ++obj->pos;
    if (obj->pos == sh3) obj->function_p = new_function_p;
    return x;
}

static uint64_t case_1(MelgState *obj)
{
    return case_generic(obj, MM, LAG1, NN - MM, case_2);
}

static uint64_t case_2(MelgState *obj)
{
    return case_generic(obj, MM - NN, LAG1, LAG1over, case_3);
}

static uint64_t case_3(MelgState *obj)
{
    return case_generic(obj, MM - NN, -LAG1over, NN - 1, case_4);
}

static uint64_t case_4(MelgState *obj)
{
    uint64_t x = (obj->melg[NN-1] & MASKU) | (obj->melg[0] & MASKL);
    obj->lung = (x >> 1) ^ mag01[(int)(x & 1ULL)] ^ obj->melg[MM-1] ^ MAT3NEG(37, obj->lung);
    obj->melg[NN-1] = x ^ MAT3POS(14, obj->lung);
    x = obj->melg[obj->pos] ^ (obj->melg[obj->pos] << SHIFT1);
    x = x ^ (obj->melg[obj->pos - LAG1over] & MASK1);
    obj->pos = 0;
    obj->function_p = case_1;
    return x;
}

static inline uint64_t get_bits_raw(MelgState *obj)
{
    return obj->function_p(obj);
}

static void *create(const CallerAPI *intf)
{
    MelgState *obj = intf->malloc(sizeof(MelgState));
    MelgState_init(obj, intf->get_seed64());
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[8] = {
        0x3FFF7AB991AC2BF9, 0xF948868BC5F984BF,
        0xF5275F657D3FFF28, 0xB4A5B1E034F06590,
        0xEAF5841B0617A2C5, 0xDF5288767154C7AC,
        0x27CBF48B5B7EB639, 0xA7DA4F31AA37C0F5
    };
    int is_ok = 1;
    MelgState *obj = intf->malloc(sizeof(MelgState));
    MelgState_init(obj, 1234567890);
    for (int i = 0; i < 100000; i++) {
        (void) get_bits_raw(obj);
    }
    intf->printf("%16s %16s\n", "Output", "Reference");
    for (int i = 0; i < 8; i++) {
        uint64_t u = get_bits_raw(obj);
        intf->printf("%16llX %16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("Melg44497", run_self_test)
