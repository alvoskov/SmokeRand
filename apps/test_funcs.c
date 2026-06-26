/**
 * @file test_funcs.c
 * @brief Tests for some special functions implemented in SmokeRand.
 * @details These special functions can be divided into three groups:
 *
 * - Sorting subroutines: radix sort and quicksort.
 * - Reimplementation of C99 specific mathematical functions.
 * - C.d.f, c.c.d.f. and p.d.f. for some distributions.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/core.h"
#include "smokerand/specfuncs.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

void print_is_ok(int is_ok)
{
    if (is_ok) {
        printf("--- Passed ---\n\n");
    } else {
        printf("--- Failed ---\n\n");
    }
}


int is_array64_sorted(const uint64_t *x, size_t len)
{
    int is_sorted = 1;
    for (size_t i = 0; i < len - 1; i++) {
        if (x[i] > x[i + 1]) {
            is_sorted = 0;
            break;
        }
    }
    return is_sorted;
}

static void fill_rand64(uint64_t *x, size_t len)
{
    uint64_t seed = (uint64_t) time(NULL);
    for (size_t i = 0; i < len; i++) {
        x[i] = pcg_bits64(&seed);
    }
}


static int cmp_int64(const void *aptr, const void *bptr)
{
    uint64_t aval = *((uint64_t *) aptr), bval = *((uint64_t *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}

static void qsort64_wrap(uint64_t *x, size_t len)
{
    qsort(x, len, sizeof(uint64_t), cmp_int64);
}


typedef struct {
    const char *name;
    void (*run)(uint64_t *, size_t);
} SortMethodInfo;

int test_radixsort64()
{
    const size_t len = 1U << 25;
    uint64_t *x = calloc(len, sizeof(uint64_t));
    int is_ok = 1;
    const SortMethodInfo methods[] = {
        {"radixsort64_inplace", radixsort64_inplace},
        {"quicksort64", quicksort64},
        {"qsort64", qsort64_wrap},        
        {NULL, NULL}
    };


    for (const SortMethodInfo *ptr = methods; ptr->name != NULL; ptr++) {
        fill_rand64(x, len);
        clock_t tic = clock();
        ptr->run(x, len);
        clock_t toc = clock();
        double msec = ((double) (toc - tic)) / CLOCKS_PER_SEC * 1000;
        printf("%s --- time elapsed: %g ms\n", ptr->name, msec);
        if (is_array64_sorted(x, len)) {
            printf("%s: array is sorted\n", ptr->name);
        } else {
            printf("%s: array is not sorted\n", ptr->name);
            is_ok = 0;
        }

        for (size_t i = 0; i < len; i++) {
            x[i] = 0;
        }
        tic = clock();
        ptr->run(x, len);
        toc = clock();
        msec = ((double) (toc - tic)) / CLOCKS_PER_SEC * 1000;
        printf("%s|empty --- time elapsed: %g ms\n", ptr->name, msec);        
    }
    free(x);
    return is_ok;
}

int test_chi2()
{
    double cdf_data[] = { //x, p, cdf
        1e-10,      1.0,        7.978845607895684e-06,
        0.01,       1.0,        0.079655674554058,
        0.5,        1.0,        0.520499877813046,
        20,         1.0,        0.999992255783569,
        1e-4,       2.0,        4.999875002083312e-05,
        1.9,        2.0,        0.613258976545499,
        16.9,       2.0,        0.999786099584632,
        1.0,        10.0,       1.721156299558406e-04,
        30.0,       10.0,       0.999143358789225,
        1.0,        11.0,       5.038994868783313e-05,
        35.0,       11.0,       0.999752198728789,
        101.0,      100.0,      0.546807776723632,
        101.0,      101.0,      0.518714954630780,
        1.0,        100.0,      1.788776510435092e-80,
        1.0,        101.0,      1.775328422787826e-81,
        1200.0,     2000.0,     2.324060057960840e-50,
        3500.0,     5000.0,     7.752231912964759e-64,
        12500.0,    15000.0,    2.809079834374340e-53,
        45000.0,    50000.0,    1.582451207342186e-60,
        92000.0,    99999.0,    8.600953750430389e-76,
        100200.0,   99999.0,    0.673876059742346,
        92000.0,    100000.0,   8.248781517546660e-76,
        300000.0,   300000.0,   0.500343354858959,
        290000.0,   300000.0,   2.960662829110295e-39,
        0.0
    };

    double ccdf_data[] = {
        550.0,      101.0,      5.878272935778362e-63,
        550.0,      100.0,      2.494748793996068e-63,
        0.0
    };

    int is_ok = 1;
    static const double RELERR_MAX = 1e-10;

    printf("----- test_chi2 -----\n");
    printf("chi2cdf test\n");
    printf("%10s %10s %16s %16s %10s %16s\n",
        "x", "f", "xref", "xcalc", "relerr", "x+xc-1");
    for (size_t i = 0; cdf_data[i] != 0; i += 3) {
        double x = cdf_data[i];
        unsigned long f = (unsigned long) cdf_data[i + 1];
        double x_ref = cdf_data[i + 2];
        double x_calc = sr_chi2_cdf(x, f);
        double xc_calc = sr_chi2_pvalue(x, f);
        double relerr = fabs((x_calc - x_ref) / x_ref);
        double sum_m_1 = x_calc + xc_calc - 1.0;
        printf("%10g %10lu %16g %16g %10.2g %16g\n",
            x, f, x_ref, x_calc, relerr, sum_m_1);
        if ((relerr > RELERR_MAX || fabs(sum_m_1) > RELERR_MAX) && f < 50000) {
            is_ok = 0;
        }
    }

    printf("chi2ccdf test\n");
    printf("%10s %8s %16s %16s %16s\n",
        "x", "f", "xcref", "xccalc", "x+xc-1");
    for (size_t i = 0; ccdf_data[i] != 0; i += 3) {
        double x = ccdf_data[i];
        unsigned long f = (unsigned long) ccdf_data[i + 1];
        double xc_ref = ccdf_data[i + 2];
        double x_calc = sr_chi2_cdf(x, f);
        double xc_calc = sr_chi2_pvalue(x, f);
        double relerr = fabs((xc_calc - xc_ref) / xc_ref);
        double sum_m_1 = x_calc + xc_calc - 1.0;
        printf("%10g %10lu %16g %16g %16g\n",
            x, f, xc_ref, xc_calc, sum_m_1);
        if (relerr > RELERR_MAX || fabs(sum_m_1) > RELERR_MAX) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

/**
 * @brief Test for Kolmogorov-Smirnov ccdf function based on values
 * from `scipy.special.kolmogorov`.
 */
int test_ks()
{
    const double k_data[] = {
        0.3,    0.9999906941986655,
        0.5,    0.9639452436648751,
        0.9,    0.3927307079406543,
        1.0,    0.26999967167735456,
        1.1,    0.1777181926064012,
        2.0,    0.0006709252557796953,
        5.0,    3.8574996959278356e-22,
        10.0,   2.767793053473475e-87,
        0.0
    };
    int is_ok = 1;
    printf("----- test_ks ----- \n");
    printf("%16s %16s %16s %16s\n", "k", "fref", "fcalc", "relerr,%");
    for (int i = 0; k_data[i] != 0.0; i += 2) {
        double k = k_data[i];
        double ccdf_ref = k_data[i + 1];
        double ccdf_calc = sr_ks_pvalue(k);
        double relerr = (ccdf_calc - ccdf_ref)/ccdf_ref;
        if (fabs(relerr) > 1e-15) {
            is_ok = 0;
        }
        printf("%16g %16g %16g %16g\n",
            k, ccdf_ref, ccdf_calc, 100*relerr);
    }
    print_is_ok(is_ok);
    return is_ok;
}

/**
 * @brief Test the implementation of geometric distribution
 * @details The next Python 3.x code can be used for test vectors generation:
 *
 *    from scipy.stats import geom
 *    p_vec = [1e-50, 1e-10, 1e-4, 0.01, 0.25, 0.5, 0.75, 0.99, 0.9999, 1 - 1e-10]
 *    x_vec = [1, 2, 5, 6, 10, 101, 10000, 1000000]
 *    for p in p_vec:
 *        for x in x_vec:
 *            cdf, ccdf = geom.cdf(x, p), geom.sf(x, p)
 *            print(f"        {p:15.15g}, {x:10g}, {cdf:20.15g}, {ccdf:20.15g},")
 *
 */
static int test_geom()
{
    const double data[] = {
            //     p           t                   cdf           ccdf/pvalue
               1e-50,          1,                1e-50,                    1,
               1e-50,          2,                2e-50,                    1,
               1e-50,          5,                5e-50,                    1,
               1e-50,          6,                6e-50,                    1,
               1e-50,         10,                1e-49,                    1,
               1e-50,        101,             1.01e-48,                    1,
               1e-50,      10000,                1e-46,                    1,
               1e-50,      1e+06,                1e-44,                    1,
               1e-10,          1,                1e-10,         0.9999999999,
               1e-10,          2,     1.9999999999e-10,         0.9999999998,
               1e-10,          5,      4.999999999e-10,         0.9999999995,
               1e-10,          6,     5.9999999985e-10,         0.9999999994,
               1e-10,         10,     9.9999999955e-10,          0.999999999,
               1e-10,        101,    1.00999999495e-08,         0.9999999899,
               1e-10,      10000, 9.99999500050167e-07,      0.9999990000005,
               1e-10,      1e+06,  9.9995000171662e-05,    0.999900004999828,
              0.0001,          1,               0.0001,               0.9999,
              0.0001,          2,           0.00019999,           0.99980001,
              0.0001,          5,   0.0004999000099995,        0.99950009999,
              0.0001,          6,   0.0005998500199985,    0.999400149980001,
              0.0001,         10, 0.000999550119979003,    0.999000449880021,
              0.0001,        101,   0.0100496662424983,    0.989950333757502,
              0.0001,      10000,     0.63213895356707,     0.36786104643293,
              0.0001,      1e+06,                    1,  3.7015207857526e-44,
                0.01,          1,                 0.01,                 0.99,
                0.01,          2,               0.0199,               0.9801,
                0.01,          5,         0.0490099501,         0.9509900499,
                0.01,          6,       0.058519850599,       0.941480149401,
                0.01,         10,   0.0956179249911955,    0.904382075008805,
                0.01,        101,    0.637627982139503,    0.362372017860497,
                0.01,      10000,                    1, 2.24877484981648e-44,
                0.01,      1e+06,                    1,                    0,
                0.25,          1,                 0.25,                 0.75,
                0.25,          2,               0.4375,               0.5625,
                0.25,          5,         0.7626953125,         0.2373046875,
                0.25,          6,       0.822021484375,       0.177978515625,
                0.25,         10,    0.943686485290527,   0.0563135147094727,
                0.25,        101,    0.999999999999759, 2.40540163903614e-13,
                0.25,      10000,                    1,                    0,
                0.25,      1e+06,                    1,                    0,
                 0.5,          1,                  0.5,                  0.5,
                 0.5,          2,                 0.75,                 0.25,
                 0.5,          5,              0.96875,              0.03125,
                 0.5,          6,             0.984375,             0.015625,
                 0.5,         10,         0.9990234375,         0.0009765625,
                 0.5,        101,                    1, 3.94430452610505e-31,
                 0.5,      10000,                    1,                    0,
                 0.5,      1e+06,                    1,                    0,
                0.75,          1,                 0.75,                 0.25,
                0.75,          2,               0.9375,               0.0625,
                0.75,          5,         0.9990234375,         0.0009765625,
                0.75,          6,       0.999755859375,       0.000244140625,
                0.75,         10,    0.999999046325684,  9.5367431640625e-07,
                0.75,        101,                    1, 1.55575381946527e-61,
                0.75,      10000,                    1,                    0,
                0.75,      1e+06,                    1,                    0,
                0.99,          1,                 0.99,                 0.01,
                0.99,          2,               0.9999,               0.0001,
                0.99,          5,         0.9999999999,                1e-10,
                0.99,          6,       0.999999999999,                1e-12,
                0.99,         10,                    1, 1.00000000000001e-20,
                0.99,        101,                    1, 1.00000000000005e-202,
                0.99,      10000,                    1,                    0,
                0.99,      1e+06,                    1,                    0,
              0.9999,          1,               0.9999, 9.99999999999889e-05,
              0.9999,          2,           0.99999999, 9.99999999999778e-09,
              0.9999,          5,                    1, 9.99999999999445e-21,
              0.9999,          6,                    1, 9.99999999999334e-25,
              0.9999,         10,                    1,  9.9999999999889e-41,
              0.9999,        101,                    1,                    0,
              0.9999,      10000,                    1,                    0,
              0.9999,      1e+06,                    1,                    0,
        0.9999999999,          1,         0.9999999999, 1.00000008274037e-10,
        0.9999999999,          2,                    1, 1.00000016548075e-20,
        0.9999999999,          5,                    1, 1.00000041370192e-50,
        0.9999999999,          6,                    1, 1.00000049644233e-60,
        0.9999999999,         10,                    1, 1.00000082740401e-100,
        0.9999999999,        101,                    1,                    0,
        0.9999999999,      10000,                    1,                    0,
        0.9999999999,      1e+06,                    1,                    0,
                 0.0
    };
    int is_ok = 1;
    printf("----- test_geom ----- \n");
    printf("%16s %16s  %16s %16s %16s  %16s %16s %16s\n",
        "p", "t",
        "cdf_ref", "cdf_calc", "relerr,%",
        "ccdf_ref", "ccdf_calc", "relerr,%");
    for (int i = 0; data[i] != 0.0; i += 4) {
        const double p = data[i];
        const unsigned long t = (unsigned long) data[i + 1];        
        const double cdf_ref = data[i + 2];
        const double ccdf_ref = data[i + 3];
        const double cdf_calc = sr_geom_cdf(t, p);
        const double ccdf_calc = sr_geom_ccdf(t, p);
        const double cdf_relerr = (cdf_calc - cdf_ref) / (cdf_ref + DBL_MIN);
        const double ccdf_relerr = (ccdf_calc - ccdf_ref) / (ccdf_ref + DBL_MIN);
        if (fabs(cdf_relerr) > 1e-12 || fabs(ccdf_relerr) > 1e-12) {
            is_ok = 0;
        }
        printf("%14g %10lu  %20.14g %20.14g %8.2g  %20.14g %20.14g %8.2g\n",
            p, t,
            cdf_ref, cdf_calc, 100.0 * cdf_relerr,
            ccdf_ref, ccdf_calc, 100.0 * ccdf_relerr);
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_hamming_weights()
{
    uint64_t x = 0xDEADBEEFDEADBEEF;
    printf("----- test_hamming -----\n");
    int hw = (int) get_uint64_hamming_weight(x), hw_ref = 48;
    printf("hamming weight = %d (ref.value is %d)\n", hw, hw_ref);
    print_is_ok(hw == hw_ref);
    return hw == hw_ref;
}

int test_binopdf()
{
    static const double ref_256[] = {
        1.0, 8.0, 28.0, 56.0, 70.0, 56.0, 28.0, 8.0, 1.0        
    };
    static const double ref_4_pow_9[] = {
        19683.0, 59049.0, 78732.0, 61236.0, 30618.0, 10206.0, 2268.0, 324.0, 27.0
    };
    static const double MAX_RELERR = 1e-14;
    int is_ok = 1;
    printf("----- test_binopdf -----\n");
    printf("Part 1. n = 8, p = 0.5\n");
    printf("%3s %3s %10s %10s %10s\n", "k", "n", "calc", "ref", "relerr%");
    for (unsigned int i = 0; i < 8; i++) {
        double coeff_calc = sr_binomial_pdf(i, 8, 0.5) * 256.0;
        double coeff_ref = ref_256[i];
        double relerr = fabs((coeff_calc - coeff_ref) / coeff_ref);
        printf("%3d %3d %10g %10g %10.2g\n", i, 8, coeff_calc, coeff_ref, 100*relerr);
        if (relerr > MAX_RELERR) {
            is_ok = 0;
        }
    }
    printf("\n");
    printf("Part 2. n = 8, p = 0.5\n");
    printf("%3s %3s %10s %10s %10s\n", "k", "n", "calc", "ref", "relerr%");
    for (unsigned int i = 0; i < 9; i++) {
        double coeff_calc = sr_binomial_pdf(i, 9, 0.25) * pow(4.0, 9.0);
        double coeff_ref = ref_4_pow_9[i];
        double relerr = fabs((coeff_calc - coeff_ref) / coeff_ref);
        printf("%3d %3d %10g %10g %10.2g\n", i, 8, coeff_calc, coeff_ref, 100*relerr);
        if (relerr > MAX_RELERR) {
            is_ok = 0;
        }
    }
    printf("Part 3. Some arbitrary calls\n");
    {
        static const double p_ref_1 = 0.0, p_ref_2 = 0.013762612203149;
        double p_calc_1 = sr_binomial_pdf(971, 1493, 0.036356);
        double p_calc_2 = sr_binomial_pdf(128, 256, 0.45);
        double abserr_1 = fabs(p_calc_1 - p_ref_1);
        double abserr_2 = fabs(p_calc_2 - p_ref_2);
        printf("%25s %25s %10s\n", "pcalc", "pref", "abserr");
        printf("%25.16g %25.16g %10.2g\n", p_calc_1, p_ref_1, abserr_1);
        printf("%25.16g %25.16g %10.2g\n", p_calc_2, p_ref_2, abserr_2);
        if (abserr_1 > 1e-15 || abserr_2 > 1e-15) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_expm1()
{
    static const double xref[] = {
        -5.0, -0.5, -0.01, -0.001, -1e-14,
        1e-14, 0.001, 0.01, 0.5, 5.0
    };
    int is_ok = 1;
    printf("----- test_expm1 -----\n");
    printf("%7s %25s %25s\n", "x", "f", "fref");
    for (int i = 0; i < 10; i++) {
        double x = xref[i];
        double f = sr_expm1(x), fref = expm1(x);
        printf("%7.3f %25.16g %25.16g\n", x, f, fref);
        if (fabs(f - fref) > 1e-15) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

static int test_lgamma_x(double x)
{
    double f = sr_lgamma(x), fref = lgamma(x);
    double relerr = fabs((f - fref) / fref);
    printf("%8.0f %25.16g %25.16g %10.3g\n", x, f, fref, relerr);
    if ((fref < 1e100 || f < 1e100) && relerr > 1e-14) {
        return 0;
    } else {
        return 1;
    }
}

int test_lgamma()
{
    int is_ok = 1;
    printf("----- test_lgamma -----\n");
    printf("%8s %25s %25s %10s\n", "x", "f", "fref", "relerr");
    for (int i = 0; i < 10; i++) {
        is_ok = is_ok & test_lgamma_x(i);
    }
    for (int i = 260; i < 280; i++) {
        is_ok = is_ok & test_lgamma_x(i);
    }
    is_ok = is_ok & test_lgamma_x(100000.0);
    print_is_ok(is_ok);
    return is_ok;
}

int test_stdnorm()
{
    static const double ref[] = {
        -36.0,  4.182624065797386e-284,
        -5.0,   2.866515718791946e-07,
        -1.0,   1.586552539314571e-01,
         0.0,   5.000000000000000e-01,
         1.0,   8.413447460685429e-01,
         5.0,   9.999997133484281e-01
    };
    int is_ok = 1;
    printf("----- test_stdnorm ------\n");
    printf("%7s %25s %25s %8s %8s\n", "x", "pcalc", "pref", "sum-1", "relerr");
    for (int i = 0; i < 5; i++) {
        double x = ref[2*i], pref = ref[2*i + 1];
        double pcalc = sr_stdnorm_cdf(x);
        double sum_m_1 = pcalc + sr_stdnorm_pvalue(x) - 1.0;
        double relerr = fabs((pcalc - pref) / pref);
        printf("%7.3f %25.16g %25.16g %8.2e %8.2e\n",
            x, pcalc, pref, sum_m_1, relerr);
        if (relerr > 1e-13 || sum_m_1 > 1e-15) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_tdistr_cdf()
{
    static double dat[] = { // t, f, p
        -1e11,  1,  3.183098861837907e-12,
        -3e9,   1,  1.061032953945969e-10,
        -1.1e5, 1,  2.893726237954743e-06,
        -5.0e4, 1,  6.366197722826988e-06,
         1e11,  1,  0.999999999996817,
        -1,     1,  0.25,
         0,     1,  0.5,
         0,     2,  0.5,
         0,     5,  0.5,
         0,     100,  0.5,
         0,     1e6,  0.5,
         1,     1,  0.75,
        -1e10,  2,  4.999999999999996e-21,
        -80000, 2,  7.812499998168966e-11,
        -50,   10, 1.237155164651344e-13,
         50,   10, 0.999999999999876,
        -30,   11, 3.333465610682530e-12,
         30,   11, 0.999999999996667,
        -8,    100, 1.136432403864001e-12,
         8,    100, 0.999999999998864,
        -1,    10,  0.170446566151030,
        -1,    11,  0.169400348098101,
         1,    10,  0.829553433848970,
         1,    11,  0.830599651901899,
        -1.96, 1e4, 0.025011760115899,
         1.96, 1e4,  0.974988239884101,
         1e10, 100.0};
    double dfmax = -1, dfmean = 0.0;
    size_t indmax = 0, n = 0;
    printf("----- test_tdistr_cdf -----\n");
    printf("%10s %8s %15s %15s %15s %10s\n",
        "t", "df", "pcalc", "pref", "pcalc_ccdf", "delta");
    for (size_t i = 0; dat[3*i] != 1e10; i++) {
        double t = dat[3*i];
        unsigned long f = (unsigned long) dat[3*i + 1];
        double p = dat[3*i + 2];
        double delta = fabs(p - sr_t_cdf(t, f)) / p;
        if (dfmax < delta) {
            dfmax = delta; indmax = i;
        }
        dfmean += delta; n++;
        printf("%10g %8lu %15g %15g %15g %10.3g\n",
            t, f, sr_t_cdf(t, f), p, sr_t_pvalue(t, f), delta);
    }
    dfmean /= (double) n;
    printf("test_tdistr_cdf; df(mean): %g; df(max): %g; ind(max): %d\n",
        dfmean, dfmax, (int) indmax);
    int is_ok = (dfmax < 1.0e-10);
    print_is_ok(is_ok);    
    return is_ok;
}


int test_halfnormal()
{
    int is_ok = 1;
    printf("----- test_halfnormal -----\n");
    printf("%10s %25s %25s %10s\n", "x", "pcalc", "pref", "relerr%");
    for (double x = -3.0; x <= 3.0; x += 0.5) {
        double pcalc = sr_halfnormal_pvalue(x);
        double pref = erfc(x / sqrt(2.0));
        double relerr = fabs((pcalc - pref) / pref);
        printf("%10.5f %25.16g %25.16g %10.3g\n",
            x, pcalc, pref, 100 * relerr);
        if (relerr > 1e-14) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_binocdf()
{
    static const double data[][4] = {
        {5.0,  10.0,     0.45, 0.738437299245508},
        {128.0, 256.0,   0.5,  0.524909554967906},
        {95.0, 100000.0, 1e-3, 0.331101644198284},
        {0, 0, 0, 0}
    };

    printf("----- test_binocdf -----\n");
    printf("%6s %6s %25s %25s %10s %10s\n",
        "k", "n", "fcalc", "fref", "relerr%", "f+fc-1");
    for (int i = 0; data[i][0] != 0; i++) {
        unsigned long k = (unsigned long) data[i][0];
        unsigned long n = (unsigned long) data[i][1];
        double p = data[i][2], f_ref = data[i][3];
        double f_calc = sr_binomial_cdf(k, n, p);
        double fc_calc = sr_binomial_pvalue((double) k, (double) n, p);
        double relerr = fabs((f_calc - f_ref) / f_ref);
        double sum_m_1 = f_calc + fc_calc - 1.0;        
        printf("%6ld %6lu %25.16g %25.16g %10.3g %10.3g\n",
            k, n, f_calc, f_ref, 100*relerr, sum_m_1);
    }
    return 0;
}

int test_norminv()
{
    int is_ok = 1;
    static const double data[][2] = {
        {1e-50,  -14.933337534788491},
        {1e-10, -6.361340902404057},
        {0.75,   0.674489750196082},
        {1000.0, 0.0}
    };    


    printf("----- test_norminv -----\n");
    printf("%10s %25s %25s %8s\n", "dx", "f(0.5-dx)", "f(0.5+dx)", "abserr");
    for (double lg_dx = -10; lg_dx < -0.99; lg_dx += 1.0) {
        double dx = pow(10.0, lg_dx);
        double f_left = sr_stdnorm_inv(0.5 - dx);
        double f_right = sr_stdnorm_inv(0.5 + dx);
        double abserr = fabs(f_left + f_right);
        printf("%10.4g %25.16g %25.16g %8.2g\n", dx, f_left, f_right, abserr);
        if (abserr > 1e-15) {
            is_ok = 0;
        }
    }
    printf("\n%10s %25s %25s %8s\n", "p", "f_ref", "f_calc", "relerr%");
    for (int i = 0; data[i][0] < 1.0; i++) {
        double p = data[i][0], f_ref = data[i][1];
        double f_calc = sr_stdnorm_inv(p);
        double relerr = fabs((f_calc - f_ref) / f_ref);
        printf("%10.4g %25.16g %25.16g %8.2g\n", p, f_ref, f_calc, relerr*100);
        if (relerr > 1e-15) {
            is_ok = 0;
        }
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_round()
{
    static const double data[][2] = {
        {-3.0, -3.0}, {-3.3, -3.0}, {-3.8, -4.0}, {-0.6, -1.0}, {-0.5, -1.0},
        {-0.4,  0.0}, { 0.0,  0.0}, { 0.1,  0.0}, { 0.4,  0.0}, { 0.5,  1.0},
        { 0.6,  1.0}, { 5.1,  5.0}, { 5.5,  6.0}, { 5.9,  6.0},
        {-10000.0, -10000.0}
    };
    int is_ok = 1;

    printf("----- test_round -----\n");
    printf("%8s %8s %8s\n", "x", "xrnd", "xrnd_ref");
    for (int i = 0; data[i][0] > -10000.0; i++) {
        double x = data[i][0], xrnd_ref = data[i][1];
        double xrnd = sr_round(x);
        if (xrnd != xrnd_ref) {
            is_ok = 0;
        }
        printf("%8g %8g %8g\n", x, xrnd, xrnd_ref);
    }
    print_is_ok(is_ok);
    return is_ok;
}

int test_linearcomp_cdf()
{
    static const double data[][3] = {
        {0.010417, -2.5,    -10000.0},
        {0.03125,  -1.5,    -2.5},
        {0.125,    -0.5,    -1.5},
        {0.5,       0.5,    -0.5},
        {0.25,      1.5,     0.5},
        {0.0625,    2.5,     1.5},
        {0.020833,  10000.0, 2.5},
        {0, 0, 0}
    };
    int is_ok = 1;
    printf("----- test_linearcomp_cdf -----\n");   
    printf("%10s %10s %8s\n", "Tref", "Tcalc", "dT");
    for (int i = 0; data[i][0] != 0; i++) {
        double Tref = data[i][0], xhigh = data[i][1], xlow = data[i][2];
        double f_low = sr_linearcomp_Tcdf(xlow);
        double f_high = sr_linearcomp_Tcdf(xhigh);
        double Tcalc = f_high - f_low;
        double dT = Tcalc - Tref;
        if (dT > 1e-6) {
            is_ok = 0;
        }
        printf("%10.7f %10.7f %8.2g\n", Tref, Tcalc, dT);
    }   
    print_is_ok(is_ok);
    return is_ok;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: test_funcs test_group\n");
        printf("  test_group: sort, specfuncs, distr\n");
        return 0;
    }
    int is_ok = 1;
    if (!strcmp(argv[1], "sort")) {
        is_ok = is_ok & test_radixsort64();
    } else if (!strcmp(argv[1], "specfuncs")) {
        is_ok = is_ok & test_expm1();
        is_ok = is_ok & test_lgamma();
        is_ok = is_ok & test_hamming_weights();
        is_ok = is_ok & test_round();
    } else if (!strcmp(argv[1], "distr")) {
        is_ok = is_ok & test_chi2();
        is_ok = is_ok & test_ks();
        is_ok = is_ok & test_binopdf();
        is_ok = is_ok & test_stdnorm();
        is_ok = is_ok & test_halfnormal();
        is_ok = is_ok & test_tdistr_cdf();
        is_ok = is_ok & test_binocdf();
        is_ok = is_ok & test_norminv();
        is_ok = is_ok & test_linearcomp_cdf();
        is_ok = is_ok & test_geom();
    } else {
        fprintf(stderr, "Unknown test group '%s'\n", argv[1]);
        is_ok = 0;
    }
    if (is_ok) {
        printf("===== PASSED =====\n");
    } else {
        printf("===== FAILED =====\n");
    }
    return (is_ok) ? 1 : 0;
}
