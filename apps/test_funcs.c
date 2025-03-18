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
    uint64_t seed = time(NULL);
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
    size_t len = 1 << 25;
    uint64_t *x = calloc(len, sizeof(uint64_t));
    clock_t tic, toc;
    double msec;
    int is_ok = 1;
    const SortMethodInfo methods[] = {
        {"radixsort64", radixsort64},
        {"quicksort64", quicksort64},
        {"qsort64", qsort64_wrap},
        {NULL, NULL}
    };


    for (const SortMethodInfo *ptr = methods; ptr->name != NULL; ptr++) {
        fill_rand64(x, len);
        tic = clock();
        ptr->run(x, len);
        toc = clock();
        msec = ((double) (toc - tic)) / CLOCKS_PER_SEC * 1000;
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
        0.0
    };

    double ccdf_data[] = {
        550.0,      101.0,      5.878272935778362e-63,
        550.0,      100.0,      2.494748793996068e-63,
        0.0
    };

    printf("chi2cdf test\n");
    printf("%10s %10s %16s %16s %16s\n",
        "x", "f", "xref", "xcalc", "x+xc-1");
    for (size_t i = 0; cdf_data[i] != 0; i += 3) {
        double x = cdf_data[i];
        unsigned long f = (unsigned long) cdf_data[i + 1];
        double x_ref = cdf_data[i + 2];
        double x_calc = sr_chi2_cdf(x, f);
        double xc_calc = sr_chi2_pvalue(x, f);
        printf("%10g %10lu %16g %16g %16g\n",
            x, f, x_ref, x_calc, x_calc + xc_calc - 1.0);
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
        printf("%10g %10lu %16g %16g %16g\n",
            x, f, xc_ref, xc_calc, x_calc + xc_calc - 1.0);
    }
    return 0;
}

/**
 * @brief Test for Kolmogorov-Smirnov ccdf function based on values
 * from `scipy.special.kolmogorov`.
 */
int test_ks()
{
    double k_data[] = {
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
    for (int i = 0; i < 8; i++) {
        printf("%3d %3d %6g\n", i, 8, sr_binomial_pdf(i, 8, 0.5) * 256.0);
    }
    printf("\n");
    for (int i = 0; i < 9; i++) {
        printf("%3d %3d %6g\n", i, 8, sr_binomial_pdf(i, 9, 0.25) * pow(4.0, 9.0));
    }
    printf("\n");
    return 0;
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

int test_lgamma()
{
    int is_ok = 1;
    printf("----- test_lgamma -----\n");
    printf("%8s %25s %25s\n", "x", "f", "fref");
    for (int i = 0; i < 10; i++) {
        double f = sr_lgamma(i), fref = lgamma(i);
        printf("%8d %25.16g %25.16g\n", i, f, fref);
        if ((fref < 1e100 || f < 1e100) && fabs(f - fref) > 1e-12) {
            is_ok = 0;
        }
    }
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
    double x = 2.8;
    printf("%25.16g %25.16g\n", sr_halfnormal_pvalue(x), erfc(x / sqrt(2.0)));
        
    return 0;
}

int test_binocdf()
{
    printf("----- test_binocdf -----\n");
    printf("%g %g %g\n", sr_binomial_cdf(5,10,0.45), 0.738437299245508,
        sr_binomial_cdf(5,10,0.45) + sr_binomial_pvalue(5,10,0.45));
    printf("%g %g %g\n", sr_binomial_cdf(95,100000,1e-3), 0.331101644198284,
        sr_binomial_cdf(95,100000,1e-3) + sr_binomial_pvalue(95,100000,1e-3));

    printf("----- test_binopdf -----\n");
    printf("%g %g\n", sr_binomial_pdf(971, 1493, 0.036356), 0.0);
    printf("%g %g\n", sr_binomial_pdf(128, 256, 0.45), 0.013763);

    return 0;
}

int test_norminv()
{
    printf("----- test_norminv -----\n");
    printf("%.15g %.15g\n", sr_stdnorm_inv(0.5 - 1e-8), sr_stdnorm_inv(0.5 + 1e-8));
    printf("%.15g %.15g\n", sr_stdnorm_inv(0.5 - 1e-4), sr_stdnorm_inv(0.5 + 1e-4));
    printf("%.15g %.15g\n", sr_stdnorm_inv(0.25), sr_stdnorm_inv(0.75));
    printf("%.15g %.15g\n", sr_stdnorm_inv(1e-10), sr_stdnorm_inv(1 - 1e-10));

    printf("%.15g\n", sr_stdnorm_inv(1e-50));

    return 0;

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
