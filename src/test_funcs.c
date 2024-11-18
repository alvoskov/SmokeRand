#include "smokerand/core.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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


int test_radixsort64()
{
    size_t len = 1 << 24;
    uint64_t *x = calloc(len, 20);
    fill_rand64(x, len);
    radixsort64(x, len);
    if (is_array64_sorted(x, len)) {
        printf("test_radixsort64: array is sorted\n");
        return 1;
    } else {
        printf("test_radixsort64: array is not sorted\n");
        return 0;
    }
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
        double x = cdf_data[i], f = cdf_data[i + 1];
        double x_ref = cdf_data[i + 2];
        double x_calc = chi2_cdf(x, f);
        double xc_calc = chi2_pvalue(x, f);
        printf("%10g %10g %16g %16g %16g\n",
            x, f, x_ref, x_calc, x_calc + xc_calc - 1.0);
    }

    printf("chi2ccdf test\n");
    printf("%10s %8s %16s %16s %16s\n",
        "x", "f", "xcref", "xccalc", "x+xc-1");
    for (size_t i = 0; ccdf_data[i] != 0; i += 3) {
        double x = ccdf_data[i], f = ccdf_data[i + 1];
        double xc_ref = ccdf_data[i + 2];
        double x_calc = chi2_cdf(x, f);
        double xc_calc = chi2_pvalue(x, f);
        printf("%10g %10g %16g %16g %16g\n",
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
    printf("test_ks\n");
    for (size_t i = 0; k_data[i] != 0.0; i += 2) {
        double k = k_data[i];
        double ccdf_ref = k_data[i + 1];
        double ccdf_calc = ks_pvalue(k);
        printf("%16g %16g %16g %16g\n",
            k, ccdf_ref, ccdf_calc,
                100*(ccdf_calc - ccdf_ref)/ccdf_ref);
    }
    return 0;
}

int test_hamming_weights()
{
    uint64_t x = 0xDEADBEEFDEADBEEF;
    printf("hamming weight = %d (ref.value is 48)\n", (int) get_uint64_hamming_weight(x));
    return 0;
}

int test_binopdf()
{
    for (int i = 0; i < 8; i++) {
        printf("%3d %3d %6g\n", i, 8, binomial_pdf(i, 8, 0.5) * 256.0);
    }
    printf("\n");
    for (int i = 0; i < 9; i++) {
        printf("%3d %3d %6g\n", i, 8, binomial_pdf(i, 9, 0.25) * pow(4.0, 9.0));
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
    printf("test_expm1\n");
    for (int i = 0; i < 10; i++) {
        double x = xref[i];
        double fref = expm1(x);
        printf("%7.3f %25.16g %25.16g\n",
            x, sr_expm1(x), fref);
    }
    return 0;
}

int test_lgamma()
{
    printf("test_lgamma\n");
    for (int i = 0; i < 10; i++) {
        printf("%25.16g %25.16g\n", sr_lgamma(i), lgamma(i));
    }
    printf("\n");
    return 0;
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

    for (int i = 0; i < 5; i++) {
        double x = ref[2*i], pref = ref[2*i + 1];
        printf("%7.3f %25.16g %25.16g %8.2e\n",
            x, stdnorm_cdf(x), pref,
            stdnorm_cdf(x) + stdnorm_pvalue(x) - 1.0);
    }
    return 0;
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
         1e100, 100.0};
    double dfmax = -1, dfmean = 0.0;
    size_t indmax = 0, n = 0;
    for (size_t i = 0; dat[3*i] != 1e100; i++) {
        double t = dat[3*i];
        unsigned long f = (unsigned long) dat[3*i + 1];
        double p = dat[3*i + 2];
        double delta = fabs(p - t_cdf(t, f)) / p;
        if (dfmax < delta) {
            dfmax = delta; indmax = i;
        }
        dfmean += delta; n++;
        printf("t=%8g; f=%6lu; pcalc=%10g; pref=%10g; pcalc_ccdf=%10g; delta=%g\n",
            t, f, t_cdf(t, f), p, t_pvalue(t, f), delta);
    }
    dfmean /= (double) n;
    printf("test_tdistr_cdf; df(mean): %g; df(max): %g; ind(max): %d\n",
        dfmean, dfmax, (int) indmax);    
    return (dfmax < 1.0e-10);
}


int test_halfnormal()
{
    double x = 2.8;
    printf("%25.16g %25.16g\n", halfnormal_pvalue(x), erfc(x / sqrt(2.0)));
        
    return 0;
}


int main()
{
    test_chi2();
    test_ks();
    test_binopdf();
    test_hamming_weights();
    test_expm1();
    test_lgamma();
    test_stdnorm();
    test_halfnormal();
    test_tdistr_cdf();
    test_radixsort64();
    return 0;
}
