/**
 * @file specfuncs.h
 * @brief Some special functions required for computation of p-values for
 * different statistical criteria.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef NAN
#define NAN 0
#endif

double sr_expm1(double x)
{
    if (x != x) {
        return x; /* NAN */
    } else if (x < -0.05 || x > 0.05) {
        return exp(x) - 1.0;
    } else {
        long double sum = 0.0, sum_old = 1.0, t = x;
        int i = 1;
        while (sum != sum_old) {
            sum_old = sum;
            sum += t;
            t *= x / (++i);
        }
        return sum;
    }
}

double sr_log2(double x)
{
    return log(x) / log(2.0);
}

/**
 * @brief Natural logarithm of gamma function, \f$ \ln\Gamma z \f$.
 * @details Based on Lancoz approximation, allows to compile SmokeRand core
 * in environments with simpilified and not fully C99 consistent math library.
 *
 * Uses Lanczos's approximation with r = 9 and n = 11. It was suggested by
 * P.Godfrey, the coefficients were recalculated by A.L.Voskov, the accuracy
 * is comparable to machine epsilon.
 *
 * References:
 *
 * - Godfrey Paul. Lanczos Implementation of the Gamma Function. 2000.
 *   http://www.numericana.com/answer/info/godfrey.htm
 * - Pugh Glendon Ralph. An analysis of the Lanczos gamma approximation.
 *   University of British Columbia. 2004. https://doi.org/10.14288/1.0080001
 */
double sr_lgamma(double x)
{
    static const long double b[] = {
        1.000000000000000174665L,     5716.400188274341378936L,
        -14815.30426768413909056L,    14291.49277657478554016L,
        -6348.160217641458813453L,    1301.608286058321874101L,
        -108.176705351436963469L,     2.605696505611755827694L,
        -0.007423452510201416131304L, 5.384136432507762221912e-08L,
        -4.023533141267529787103e-09L };
    static const long double r = 9.0;
    long double az = b[0], d = --x + r + 0.5;
    int k;
    for (k = 1; k < 11; k++) {
        az += b[k] / (x + k);
    }
    return log(az) + 0.5 * log(2*M_PI) + (x + 0.5) * log(d) - d;
}




/**
 * @brief Computation of p-values for Kolmogorov-Smirnov test.
 * @details Control (x, p) points from scipy.special.kolmogorov:
 * - (5.0, 3.8575e-22), (2.0, 0.0006709), (1.1, 0.1777),
 * - (1.0, 0.2700), (0.9, 0.3927)
 *
 * References:
 * 1. Marsaglia G., Tsang W. W., Wang J.Evaluating Kolmogorov's Distribution
 *    // Journal of Statistical Software. 2003. V. 8. N 18. P. 1-4.
 *    https://doi.org/10.18637/jss.v008.i18
 */
double ks_pvalue(double x)
{
    double xsq = x * x;
    if (x <= 0.0) {
        return 1.0;
    } else if (x > 1.0) {
        long double sum = 0.0, old_sum = 1.0, i = 1;
        int s = 1;
        while (sum != old_sum) {
            old_sum = sum;
            sum += s * exp(-2.0 * i * i * xsq);
            s = -s; i++;
        }
        return 2.0 * sum;
    } else {
        long double sum = 0.0, old_sum = 1.0, i = 1;
        long double t = -M_PI * M_PI / (8 * xsq);
        while (sum != old_sum) {
            old_sum = sum;
            sum += exp((2*i - 1) * (2*i - 1) * t);
            i++;
        }
        return 1.0 - sqrt(2 * M_PI) / x * sum;
    }
}


static double gammainc_lower_series(double a, double x)
{
    double mul = exp(-x + a*log(x) - sr_lgamma(a)), sum = 0.0;
    double t = 1.0 / a;
    int i;
    for (i = 1; i < 1000000 && t > DBL_EPSILON; i++) {
        sum += t;
        t *= x / (a + i);
    }
    return mul * sum;
}

static double gammainc_upper_contfrac(double a, double x)
{
    double mul = exp(-x + a*log(x) - sr_lgamma(a));
    double b0 = x + 1 - a, b1 = x + 3 - a;
    double p0 = b0, q0 = 1.0;
    double p1 = b1 * b0 + (a - 1.0), q1 = b1;
    double f = p1/q1, f_old = p0/q0;
    double c = p1/p0, d = q0 / q1;
    int i;
    for (i = 2; i < 1000000 && fabs(f - f_old) > DBL_EPSILON; i++) {
        f_old = f;
        double bk = x + 2*i + 1 - a;
        double ak = i * (a - i);
        if (c < 1e-30) c = 1e-30;
        c = bk + ak / c;
        d = 1.0 / (bk + ak * d);
        f *= c * d;
    }
    return mul / f;
}

/**
 * @brief An approximation of lower incomplete gamma function for
 * implementation of Poisson C.D.F. and its p-values computation.
 * @details References:
 *
 * 1. U. Blahak. Efficient approximation of the incomplete gamma function
 *    for use in cloud model applications // Geosci. Model Dev. 2010. V.3,
 *    P. 329Ö336. https://doi.org/10.5194/gmd-3-329-2010
 * 2. https://functions.wolfram.com/GammaBetaErf/Gamma2/06/02/02/
 */
double gammainc(double a, double x)
{
    if (x != x) {
        return x; /* NAN */
    } else if (x < a + 1) {
        return gammainc_lower_series(a, x);
    } else {
        return 1.0 - gammainc_upper_contfrac(a, x);
    }
}

/**
 * @brief An approximation of upper incomplemete gamma function
 */
double gammainc_upper(double a, double x)
{
    if (x != x) {
        return x; /* NAN */
    } else if (x < a + 1) {
        return 1.0 - gammainc_lower_series(a, x);
    } else {
        return gammainc_upper_contfrac(a, x);
    }
}

/**
 * @brief Natural logarithm of complete beta function: $\ln\Beta(a,b)$.
 */
double sr_betaln(double a, double b)
{
    return sr_lgamma(a) + sr_lgamma(b) - sr_lgamma(a + b);
}

/**
 * @brief Continued fraction for regularized incomplete beta function.
 * @details For internal use only.
 */
static long double betainc_frac(double x, double a, double b)
{
    long double f = 1 / (a - a*(a+b)/(a+1) * x); /* f=P1/Q1 */
    long double c = LDBL_MAX, d = f; /* c=P1/P0, d=Q0/Q1 */
    long double f_old, m;
    if (x != x) {
        return x; /* NAN */
    }
    for (f_old = -1, m = 1; f != f_old && m < 1000; m++) {
        long double abm_s, af, bf, a_2m;
        abm_s = a + b + m; a_2m = a + 2*m;
        af = (a + m - 1)*(abm_s - 1)*(b - m)*m / (a_2m - 1) / (a_2m - 1) * x*x;
        bf = a_2m + ( m*(b-m)/(a_2m - 1) - (a+m)*abm_s/(a_2m+1) ) * x;
        c = bf + af / c;
        d = 1 / (bf + af*d);
        f_old = f;
        f = f * c * d;
    }
    return f;
}

/**
 * @brief Regularized incomplete beta function $I_x(a,b)$.
 * @param x Function argument.
 * @param a Function argument.
 * @param b Function argument.
 * @param cIx_out Pointer to the output buffer for "upper"/"complementary"
 *        beta function value $1 - I_x(a,b)$.
 * @return Function value.
 * @details The implementation is based on continued fractions suggested
 * by Didonato and Morris. Its accuracy is comparable to machine epsilon.
 * References:
 *
 * 1. Didonato Armido R., Morris Alfred H. Algorithm 708: Significant Digit
 *    Computation of the Incomplete Beta Function Ratios //
 *    // ACM Trans. Math. Softw. 1992. V.18. N 3. P.360-373.
 *    https://doi.org/10.1145/131766.131776.
 * 2. Cuyt Annie, Petersen Vigdis Brevik et al. Handbook of Continued Fractions
 *    for Special Functions. 2008. Springer Dordrecht. ISBN 978-1-4020-6949-9.
 *    https://doi.org/10.1007/978-1-4020-6949-9
 */
double sr_betainc(double x, double a, double b, double *cIx_out)
{
    double Ix, cIx;
    if (x != x || x < 0 || x > 1) {
        Ix = NAN; cIx = NAN;
    } else if (x == 0) {
        Ix = 0; cIx = 1;
    } else if (x == 1) {
        Ix = 1; cIx = 0;
    } else {
        double mul = exp(a*log(x) + b*log(1 - x) - sr_betaln(a, b));
        if (x >= a / (a + b)) {
            cIx = mul * betainc_frac(1 - x, b, a);
            Ix = 1 - cIx;
        } else {
            Ix = mul * betainc_frac(x, a, b);
            cIx = 1 - Ix;
        }
    }
    if (cIx_out) *cIx_out = cIx;
    return Ix;
}

/**
 * @brief Asymptotic decomposition for t-distribution.
 * @details Based on the algoritm AS395 by Hill.
 * Reference:
 * Hill G.W. Algorithm 395: Students t-distribution // Communications of the
 * ACM. 1970. V.13. N 10. PP.617-619. https://doi.org/10.1145/355598.362775
 */
static double t_cdf_asymptotic(double x, unsigned long f)
{
    double a = (double) f - 0.5;
    double b = 48 * a * a;
    double q = x*x / (double) f;
    double z;
    if (q > 1e-5) {
        z = sqrt(a * log(1 + q));
    } else {
        z = (1 - (0.5 + (1.0/3 - (0.25 + q*q/5) * q) * q) * q) * q;
        z = sqrt(a * z);
    }
    double zsqr = z * z;
    double z_corr = z + z * (zsqr + 3) / b -
        0.4*z*(zsqr*zsqr*zsqr + 3.3*zsqr*zsqr + 24*zsqr + 85.5) /
        (10*b*(b + 0.8*zsqr*zsqr + 100));
    return (x >= 0) ? stdnorm_cdf(z_corr) : stdnorm_cdf(-z_corr);
}

/**
 * @brief Cumulative t-distribution function
 * @param x Cumulative distribution function argument
 * @param f Degrees of freedom
 * @details Based on incomplete beta function.
 */
double t_cdf(double x, unsigned long f)
{
    if (x != x || f == 0) { /* Invalid value */
        return NAN;
    } else if (f > 1000) { /* Asymptotic formula */
        return t_cdf_asymptotic(x, f);
    } else {
        double fdbl = (double) f;
        long double Ix = sr_betainc(fdbl / (x*x + fdbl), fdbl / 2.0, 0.5, NULL);
        return (double) ((x < 0) ? (0.5 * Ix) : (1 - 0.5*Ix));
    }
}


/**
 * @brief C.c.d.f. (p-value) for t-distribution
 * @param x Cumulative distribution function argument
 * @param f Degrees of freedom
 * @details Based on incomplete beta function.
 */
double t_pvalue(double x, unsigned long f)
{
    return t_cdf(-x, f);
}


/**
 * @brief Poisson distribution C.D.F. (cumulative distribution function)
 */
double poisson_cdf(double x, double lambda)
{
    return gammainc_upper(floor(x) + 1.0, lambda);
}

/**
 * @brief Calculate p-value for Poission distribution as 1 - F(x)
 * where F(x) is Poission distribution C.D.F.
 */
double poisson_pvalue(double x, double lambda)
{
    return gammainc(floor(x) + 1.0, lambda);
}


static double binomial_coeff(unsigned long n, unsigned long k)
{
    double c = 1.0;
    unsigned long i;
    for (i = 1; i <= k; i++) {
        c *= (n + 1.0 - i) / (double) i;
    }
    return c;
}

/**
 * @brief Probability function for binomial distribution.
 * @param k Number of successful attempts.
 * @param n Total number of attempts.
 * @param p Probability of success.
 */
double binomial_pdf(unsigned long k, unsigned long n, double p)
{
    double ln_pdf = log(binomial_coeff(n, k)) +
        k * log(p) + (n - k) * log(1.0 - p);
    return exp(ln_pdf);
}


/**
 * @brief Transforms a chi2-distributed variable to the normally distributed value
 * (standard normal distribution).
 * @details Based on the asymptotic approximation by Wilson and Hilferty.
 * References:
 *
 * 1. Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double chi2_to_stdnorm_approx(double x, unsigned long f)
{
    double s2 = 2.0 / (9.0 * f);
    double mu = 1 - s2;
    double z = (pow(x/f, 1.0/3.0) - mu) / sqrt(s2);
    return z;
}

/**
 * @brief Implementation of chi-square distribution c.d.f.
 * @details It is based on regularized incomplete gamma function. For very
 * large numbers of degrees of freedom asymptotic approximation from
 * `chi2emp_to_normemp_approx` is used.
 */
double chi2_cdf(double x, unsigned long f)
{
    if (f == 1) {
        return gammainc(0.5, x / 2.0); /* erf(sqrt(0.5 * x)); */
    } else if (f == 2) {
        return -sr_expm1(-0.5 * x);
    } else if (f < 100000) {
        return gammainc((double) f / 2.0, x / 2.0);
    } else {
        double z = chi2_to_stdnorm_approx(x, f);
        return stdnorm_cdf(z);
    }
}


/**
 * @brief Implementation of chi-square distribution c.c.d.f. (p-value)
 * @details It is based on regularized incomplete gamma function. For very
 * large numbers of degrees of freedom asymptotic approximation is used.
 *
 * References:
 *
 * 1. Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double chi2_pvalue(double x, unsigned long f)
{
    if (f == 1) {
        return 2.0 * stdnorm_pvalue(sqrt(x)); /* erfc(sqrt(0.5 * x)); */
    } else if (f == 2) {
        return exp(-0.5 * x);
    } else if (f < 100000) {
        return gammainc_upper((double) f / 2.0, x / 2.0);
    } else {
        double z = chi2_to_stdnorm_approx(x, f);
        return stdnorm_pvalue(z);
    }
}

/**
 * @brief Implementation of standard normal distribution p.d.f.
 */
static double stdnorm_pdf(double x)
{
    const double ln_2pi = 1.83787706640934548356; /* OEIS A061444 */
    return exp(-(x*x + ln_2pi) / 2);
}

/**
 * @brief Implementation of standard normal distribution c.d.f.
 * @details Allows to compile SmokeRand in the case of absent erf/erfc
 * functions in C standard library.
 *
 * An alternative implementation for fully C99 compilant compilers:
 * if (x > -3) {
 *     return 0.5 + 0.5 * erf(x / sqrt(2));
 * } else {
 *     return 0.5 * erfc(-x / sqrt(2));
 * }
 */
double stdnorm_cdf(double x)
{
    int i;
    if (x < -2.5) { /* Continued fractions */
        long double f = x / (x*x + 1), c = x, d = f, f_old = f - 1;
        for (i = 2; f - f_old != 0; i++) {
            c = x + i / c;
            d = 1 / (x + i*d);
            f_old = f;
            f *= c * d;
        }
        return -f * stdnorm_pdf(x);
    } else if (x < 2.5) { /* Taylor series */
        long double q = x * x / 4, a0 = 1, a1 = q, b = a0;
        for (i = 2; 1 + a0 != 1 || 1 + a1 != 1; i += 2) {
            a0 = q * (a1 - a0) / i;
            a1 = q * (a0 - a1) / (i + 1);
            b += a0 / (i + 1);
        }
        return 0.5 + x * stdnorm_pdf(x / 2) * b;
    } else if (x >= 2.5) {
        return 1.0 - stdnorm_cdf(-x);
    } else {
        return NAN;
    }
}

/**
 * @brief Implementation of standard normal distribution c.c.d.f. (p-value)
 * @details Allows to compile SmokeRand in the case of absent erf/erfc
 * functions in C standard library.
 *
 * An alternative implementation for fully C99 compilant compilers:
 *
 *     if (x > -3) {
 *         return 0.5 * erfc(x / sqrt(2));
 *     } else {
 *         return 1.0 - 0.5 * erfc(-x / sqrt(2));
 *     }
 */
double stdnorm_pvalue(double x)
{
    return stdnorm_cdf(-x);
}


/**
 * @brief Implementation of half normal distribution c.c.d.f. (p-value)
 * @details Allows to compile SmokeRand in the case of absent erf/erfc
 * functions in C standard library.
 *
 * An alternative implementation for fully C99 compilant compilers:
 *
 *     return erfc(x / sqrt(2.0));
 */
double halfnormal_pvalue(double x)
{
    return 2.0 * stdnorm_pvalue(x);
}

