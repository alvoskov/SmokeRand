/**
 * @file specfuncs.h
 * @brief Some special functions required for computation of p-values for
 * different statistical criteria.
 * @details This file is designed for C89 (ANSI C)! Don't use features
 * from C99 and later!
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
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


double sr_round(double x)
{
    double x_rnd = floor(x);
    if (x >= 0) {
        if (x - x_rnd >= 0.5) x_rnd++;
    } else if (x - x_rnd > 0.5) {
        x_rnd++;
    }
    return x_rnd;
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
double sr_ks_pvalue(double x)
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
    long i;
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
    long i;
    for (i = 2; i < 2000000 && fabs(f - f_old) > DBL_EPSILON; i++) {
        double bk = x + 2*i + 1 - a;
        double ak = i * (a - i);
        f_old = f;
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
 *    P. 329-336. https://doi.org/10.5194/gmd-3-329-2010
 * 2. https://functions.wolfram.com/GammaBetaErf/Gamma2/06/02/02/
 */
double sr_gammainc(double a, double x)
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
double sr_gammainc_upper(double a, double x)
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
 * @brief Natural logarithm of complete beta function \f$ \ln\Beta(a,b) \f$.
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
    long double f = 1 / (a - a*((long double) a+b)/(a+1.0L) * x); /* f=P1/Q1 */
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
    double z, zsqr, z_corr;
    if (q > 1e-5) {
        z = sqrt(a * log(1 + q));
    } else {
        z = (1 - (0.5 + (1.0/3 - (0.25 + q*q/5) * q) * q) * q) * q;
        z = sqrt(a * z);
    }
    zsqr = z * z;
    z_corr = z + z * (zsqr + 3) / b -
        0.4*z*(zsqr*zsqr*zsqr + 3.3*zsqr*zsqr + 24*zsqr + 85.5) /
        (10*b*(b + 0.8*zsqr*zsqr + 100));
    return (x >= 0) ? sr_stdnorm_cdf(z_corr) : sr_stdnorm_cdf(-z_corr);
}

/**
 * @brief Cumulative t-distribution function
 * @param x Cumulative distribution function argument
 * @param f Degrees of freedom
 * @details Based on incomplete beta function.
 */
double sr_t_cdf(double x, unsigned long f)
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
double sr_t_pvalue(double x, unsigned long f)
{
    return sr_t_cdf(-x, f);
}


/**
 * @brief Poisson distribution C.D.F. (cumulative distribution function)
 */
double sr_poisson_cdf(double x, double lambda)
{
    return sr_gammainc_upper(floor(x) + 1.0, lambda);
}

/**
 * @brief Calculate p-value for Poission distribution as 1 - F(x)
 * where F(x) is Poission distribution C.D.F.
 */
double sr_poisson_pvalue(double x, double lambda)
{
    return sr_gammainc(floor(x) + 1.0, lambda);
}


static double ln_binomial_coeff(unsigned long n, unsigned long k)
{
    double lnc = 0.0;
    unsigned long i;
    for (i = 1; i <= k; i++) {
        lnc += log(n + 1.0 - i) - log((double) i);
    }
    return lnc;
}


/**
 * @brief Probability function for binomial distribution.
 * @param k Number of successful attempts.
 * @param n Total number of attempts.
 * @param p Probability of success.
 */
double sr_binomial_pdf(unsigned long k, unsigned long n, double p)
{
    double ln_pdf = ln_binomial_coeff(n, k) +
        k * log(p) + (n - k) * log(1.0 - p);
    return exp(ln_pdf);
}

/**
 * @brief Probability function for binomial distribution: return all values.
 * @param n Total number of attempts.
 * @param p Probability of success.
 */
void sr_binomial_pdf_all(double *pdf, unsigned long n, double p)
{
    double lnc = 0.0, ln_pdf, ln_p = log(p), ln_1mp = log(1.0 - p);
    unsigned long i;
    for (i = 0; i <= n; i++) {
        if (i > 0) {
            lnc += log(n + 1.0 - i) - log((double) i);
        }
        ln_pdf = lnc + i * ln_p + (n - i) * ln_1mp;
        pdf[i] = exp(ln_pdf);
    }
}

/**
 * @brief Cumulative distribution function for binomial distribution.
 * @param k Number of successful attempts.
 * @param n Total number of attempts.
 * @param p Probability of success.
 */
double sr_binomial_cdf(unsigned long k, unsigned long n, double p)
{
    return sr_betainc(1 - p, n - k, 1 + k, NULL);
}

/**
 * @brief C.c.d.f. (p-value) for binomial distribution.
 * @param k Number of successful attempts.
 * @param n Total number of attempts.
 * @param p Probability of success.
 */
double sr_binomial_pvalue(double k, double n, double p)
{
    double cf;
    (void) sr_betainc(1 - p, n - k, 1 + k, &cf);
    return cf;
}

/**
 * @brief Transforms a chi2-distributed variable to the normally distributed
 * value (standard normal distribution).
 * @details Based on the asymptotic approximation by Wilson and Hilferty.
 * References:
 *
 * 1. Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double sr_chi2_to_stdnorm_approx(double x, unsigned long f)
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
double sr_chi2_cdf(double x, unsigned long f)
{
    if (f == 1) {
        return sr_gammainc(0.5, x / 2.0); /* erf(sqrt(0.5 * x)); */
    } else if (f == 2) {
        return -sr_expm1(-0.5 * x);
    } else if (f < 200000) {
        return sr_gammainc((double) f / 2.0, x / 2.0);
    } else {
        double z = sr_chi2_to_stdnorm_approx(x, f);
        return sr_stdnorm_cdf(z);
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
double sr_chi2_pvalue(double x, unsigned long f)
{
    if (f == 1) {
        return 2.0 * sr_stdnorm_pvalue(sqrt(x)); /* erfc(sqrt(0.5 * x)); */
    } else if (f == 2) {
        return exp(-0.5 * x);
    } else if (f < 100000) {
        return sr_gammainc_upper((double) f / 2.0, x / 2.0);
    } else {
        double z = sr_chi2_to_stdnorm_approx(x, f);
        return sr_stdnorm_pvalue(z);
    }
}

/**
 * @brief Implementation of standard normal distribution p.d.f.
 */
static double sr_stdnorm_pdf(double x)
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
double sr_stdnorm_cdf(double x)
{
    int i;
    if (x < -38.0) {
        return 0.0;
    } else if (x > 38.0) {
        return 1.0;
    } else if (x < -2.5) { /* Continued fractions */
        long double f = x / (x*x + 1), c = x, d = f, f_old = f - 1;
        for (i = 2; f - f_old != 0; i++) {
            c = x + i / c;
            d = 1 / (x + i*d);
            f_old = f;
            f *= c * d;
        }
        return -f * sr_stdnorm_pdf(x);
    } else if (x < 2.5) { /* Taylor series */
        long double q = x * x / 4, a0 = 1, a1 = q, b = a0;
        for (i = 2; 1 + a0 != 1 || 1 + a1 != 1; i += 2) {
            a0 = q * (a1 - a0) / i;
            a1 = q * (a0 - a1) / (i + 1);
            b += a0 / (i + 1);
        }
        return 0.5 + x * sr_stdnorm_pdf(x / 2) * b;
    } else if (x >= 2.5) {
        return 1.0 - sr_stdnorm_cdf(-x);
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
double sr_stdnorm_pvalue(double x)
{
    return sr_stdnorm_cdf(-x);
}



/**
 * @brief (Phi(x) - p) / p(x) relationship for standard normal distriubtion;
 * it is used in Newton method.
 * @param x P.d.f. and c.d.f. argument
 */
static double stdnorm_cdf_pdf_rel(double x, double p)
{
    double pdf = sr_stdnorm_pdf(x);
    if (x >= 0.0) {
        return (1 - p) / pdf - sr_stdnorm_pvalue(x) / pdf;
    } else {
        return sr_stdnorm_pvalue(-x) / pdf - p / pdf;
    }
}



/**
 * @brief Inverse cumulative distribution function for the standard normal
 * distribution.
 * @details For p close to 0.5 Taylor series are used. For all other values
 * it begins with an initial approximation by Soranzo et al. and then
 * refines it by Newton method using the (1 - PHI(x))/p(x) function by
 * Marsaglia.
 *   [1] Soranzo A., Epure E. Practical explicitly invertible approxmation
 *   to 4 decimals of normal cumulative distribution function modifying
 *   Winitzki's approximation of erf. https://doi.org/10.48550/arXiv.1211.6403
 *   [2] Marsaglia G. Evaluating the Normal Distribution. // Journal of
 *   Statisical Software. 2004. V. 11. N 4.
 *   https://doi.org/10.18637/jss.v011.i04
 */
double sr_stdnorm_inv(double p)
{
    int i;
    double pp, a, b, z, znew;
    /* Check input arguments */
    if (p < 0 || p > 1) {
        return NAN;
    } else if (p < DBL_MIN) {
        return -HUGE_VAL;
    } else if (p > 1 - 1e-16) {
        return +HUGE_VAL;
    }
    /* Taylor series expansion for p close to 0.5 */
    if (fabs(p - 0.5) < 1e-5) {
        double pp = 2*p - 1;
        return sqrt(0.5*M_PI) * pp * (1 + M_PI / 12 * pp * pp);
    }
    /* Soranzo initial approximation */
    pp = 4*p*(1 - p);
    a = log((pp < DBL_MIN) ? DBL_MIN : pp);
    b = 8.5 + a;
    z = sqrt(-b + sqrt(b * b - 26.694*a));
    if (p < 0.5) z = -z;
    /* Newton method iterations */
    znew = z - stdnorm_cdf_pdf_rel(z, p);
    for (i = 0; i < 10 && fabs(znew - z) > DBL_EPSILON; i++) {
        z = znew;
        znew -= stdnorm_cdf_pdf_rel(z, p);
    }
    return z;
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
double sr_halfnormal_pvalue(double x)
{
    return 2.0 * sr_stdnorm_pvalue(x);
}


/**
 * @brief Implementation of c.d.f. for the T variable from the linear
 * complexity test.
 * @details The next formula is used:
 *
 * \f[
 *   F(k) = \begin{cases}
 *     1 - \frac{2^{-2k + 2}}{3} & \textrm{for } k > 0 \\
 *         \frac{2^{2k + 1}}{3}  & \textrm{for } k \le 0 \\
 *   \end{cases}
 * \f]
 *
 * Suggested in the next work:
 * 
 * 1. Rukhin A., Soto J. et al. A Statistical Test Suite for Random and
 *    Pseudorandom Number Generators for Cryptographic Applications //
 *    NIST SP 800-22 Rev. 1a. https://doi.org/10.6028/NIST.SP.800-22r1a
 */
double sr_linearcomp_Tcdf(double k)
{
    k = sr_round(k);
    if (k > 0.0) {
        return 1.0 - pow(2.0, -2*k + 2.0) / 3.0;
    } else {
        return pow(2.0, 2*k + 1) / 3.0;
    }
}

/**
 * @brief Implementation of c.c.d.f. for the T variable from the linear
 * complexity test. See the `linearcomp_Tcdf` function for details.
 */
double sr_linearcomp_Tccdf(double k)
{
    k = sr_round(k);
    if (k > 0.0) {
        return pow(2.0, -2*k + 2.0) / 3.0;
    } else {
        return 1.0 - pow(2.0, 2*k + 1) / 3.0;
    }
}
