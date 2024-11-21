/**
 * @file core.c
 * @brief Subroutines and special functions required for implementation
 * of statistical tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/core.h"
#include "smokerand/entropy.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#if defined(_MSC_VER) || defined(__WATCOMC__)
#include <io.h>
#endif
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static Entropy entropy = {{0, 0, 0, 0}, 0, NULL, 0, 0};
static char cmd_param[128] = {0};
static int use_stderr_for_printf = 0;

static const char *get_cmd_param(void)
{
    return cmd_param;
}

void set_cmd_param(const char *param)
{
    strncpy(cmd_param, param, 128);
    cmd_param[127] = 0;
}

void set_use_stderr_for_printf(int val)
{
    use_stderr_for_printf = val;
}

///////////////////////////////
///// Single-threaded API /////
///////////////////////////////

static uint32_t get_seed32(void)
{
    return Entropy_seed64(&entropy, 0) >> 32;
}

static uint64_t get_seed64(void)
{
    return Entropy_seed64(&entropy, 0);
}

static int printf_ser(const char *format, ...)
{
    int ans;
    va_list args;
    va_start(args, format);
    if (use_stderr_for_printf) {
        ans = vfprintf(stderr, format, args);
    } else {
        ans = vprintf(format, args);
    }
    va_end(args);
    return ans;
}

CallerAPI CallerAPI_init(void)
{
    CallerAPI intf;
    if (entropy.state == 0) {
        Entropy_init(&entropy);
    }
    intf.get_seed32 = get_seed32;
    intf.get_seed64 = get_seed64;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_ser;
    intf.strcmp = strcmp;
    return intf;
}

void CallerAPI_free(void)
{
    Entropy_free(&entropy);
}

/////////////////////////////
///// Multithreaded API /////
/////////////////////////////

#if defined(USE_PTHREADS) || defined(USE_WINTHREADS)

#ifdef USE_PTHREADS
// Begin of pthreads-specific code
static pthread_mutex_t get_seed64_mt_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t printf_mt_mutex = PTHREAD_MUTEX_INITIALIZER;
static void init_mutexes()
{
}
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex);
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex);
static inline uint64_t get_current_thread_id()
{
    return (uint64_t) pthread_self();
}
static inline uint64_t get_thread_id(pthread_t handle)
{
    return (uint64_t) handle;
}
// End of pthreads-specific code
#else
// Begin of WinAPI-specific code
static HANDLE get_seed64_mt_mutex = NULL;
static HANDLE printf_mt_mutex = NULL;
static void init_mutexes()
{
    get_seed64_mt_mutex = CreateMutex(NULL, FALSE, "seed64_mutex");
    printf_mt_mutex = CreateMutex(NULL, FALSE, "printf_mutex");
}
#define MUTEX_LOCK(mutex) DWORD dwResult = WaitForSingleObject(mutex, INFINITE); \
    if (dwResult != WAIT_OBJECT_0) { \
        fprintf(stderr, "get_seed64_mt internal error"); \
        exit(1); \
    }
#define MUTEX_UNLOCK(mutex) ReleaseMutex(mutex);
static inline uint64_t get_current_thread_id()
{
    return (uint64_t) GetCurrentThreadId();
}
static inline uint64_t get_thread_id(DWORD id)
{
    return (uint64_t) id;
}
// End of WinAPI-specific code
#endif

static uint64_t get_seed64_mt(void)
{
    MUTEX_LOCK(get_seed64_mt_mutex);
    uint64_t seed = Entropy_seed64(&entropy, get_current_thread_id());
    MUTEX_UNLOCK(get_seed64_mt_mutex);
    return seed;
}

static uint32_t get_seed32_mt(void)
{
    return get_seed64_mt() >> 32;
}




static int printf_mt(const char *format, ...)
{
    int ans;
    MUTEX_LOCK(printf_mt_mutex);
    va_list args;
    va_start(args, format);
    if (use_stderr_for_printf) {
        fprintf(stderr, "=== THREAD #%2llu ===> ", get_current_thread_id());
        ans = vfprintf(stderr, format, args);
    } else {
        printf("=== THREAD #%2llu ===> ", get_current_thread_id());
        ans = vprintf(format, args);
    }
    va_end(args);
    MUTEX_UNLOCK(printf_mt_mutex);
    return ans;
}

/**
 * @brief Return thread-safe versions of functions
 */
CallerAPI CallerAPI_init_mthr(void)
{
    CallerAPI intf;
    if (entropy.state == 0) {
        Entropy_init(&entropy);
        init_mutexes();
    }
    intf.get_seed32 = get_seed32_mt;
    intf.get_seed64 = get_seed64_mt;
    intf.get_param = get_cmd_param;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf_mt;
    intf.strcmp = strcmp;
    return intf;
}
#else
/**
 * @brief Return one-threaded version of functions for platforms
 * that don't support pthread.h
 */
CallerAPI CallerAPI_init_mthr(void)
{
    return CallerAPI_init();
}
#endif

/////////////////////////////////////////////
///// Some platform-dependent functions /////
/////////////////////////////////////////////

int get_cpu_numcores(void)
{
#ifdef USE_LOADLIBRARY
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

////////////////////////////////////////////
///// TestResults class implementation /////
////////////////////////////////////////////

TestResults TestResults_create(const char *name)
{
    TestResults ans;
    ans.name = name;
    ans.p = NAN;
    ans.alpha = NAN;
    ans.x = NAN;
    ans.thread_id = 0;
    return ans;
}


///////////////////////////////////////
///// Some mathematical functions /////
///////////////////////////////////////



PValueCategory get_pvalue_category(double pvalue)
{
    const double pvalue_fail_val = 1.0e-10;
    const double pvalue_warn_val = 1.0e-3;
    if (pvalue < pvalue_fail_val || pvalue > 1.0 - pvalue_fail_val) {
        return pvalue_failed;
    } else if (pvalue < pvalue_warn_val || pvalue > 1.0 - pvalue_warn_val) {
        return pvalue_warning;
    } else {
        return pvalue_passed;
    }
}

const char *interpret_pvalue(double pvalue)
{
    switch (get_pvalue_category(pvalue)) {
    case pvalue_failed:
        return "FAIL";
    case pvalue_warning:
        return "SUSPICIOUS";
    case pvalue_passed:
        return "Ok";
    default:
        return "???";
    }
}


double sr_expm1(double x)
{
    if (x != x) {
        return NAN;
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
    for (int k = 1; k < 11; k++) {
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
    for (int i = 1; i < 1000000 && t > DBL_EPSILON; i++) {
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
    for (int i = 2; i < 1000000 && fabs(f - f_old) > DBL_EPSILON; i++) {
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
 *    P. 329–336. https://doi.org/10.5194/gmd-3-329-2010
 * 2. https://functions.wolfram.com/GammaBetaErf/Gamma2/06/02/02/
 */
double gammainc(double a, double x)
{
    if (x != x) {
        return NAN;
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
        return NAN;
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
    long double f = 1 / (a - a*(a+b)/(a+1) * x); // f=P1/Q1
    long double c = LDBL_MAX, d = f; // c=P1/P0, d=Q0/Q1
    if (x != x) {
        return NAN;
    }
    for (long double f_old = -1, m = 1; f != f_old && m < 1000; m++) {
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
    if (x != x || f == 0) { // Invalid value
        return NAN;
    } else if (f > 1000) { // Asymptotic formula
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
    for (unsigned long i = 1; i <= k; i++) {
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
        return 2.0 * stdnorm_pvalue(sqrt(x));//erfc(sqrt(0.5 * x));
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


//////////////////////////////////////////////////
///// Subroutines for working with C modules /////
//////////////////////////////////////////////////


/**
 * @brief Loads shared library (`.so` or `.dll`) with module that
 * implements pseudorandom number generator.
 */
GeneratorModule GeneratorModule_load(const char *libname)
{
    GeneratorModule mod = {.valid = 1};
#ifdef USE_LOADLIBRARY
    mod.lib = LoadLibraryA(libname);
    if (mod.lib == 0 || mod.lib == INVALID_HANDLE_VALUE) {
        int errcode = (int) GetLastError();
        fprintf(stderr, "Cannot load the '%s' module; error code: %d\n",
            libname, errcode);
        mod.lib = 0;
        mod.valid = 0;
        return mod;
    }
#else
    mod.lib = dlopen(libname, RTLD_LAZY);
    if (mod.lib == 0) {
        fprintf(stderr, "dlopen() error: %s\n", dlerror());
        mod.lib = 0;
        mod.valid = 0;
        return mod;
    };
#endif

    GetGenInfoFunc gen_getinfo = (GetGenInfoFunc)( (void *) DLSYM_WRAPPER(mod.lib, "gen_getinfo") );
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        mod.valid = 0;
    } else if (!gen_getinfo(&mod.gen)) {
        fprintf(stderr, "'gen_getinfo' function failed\n");
        mod.valid = 0;
    }
    return mod;
}

/**
 * @brief Unloads shared library (`.so` or `.dll`) with module that
 * implements pseudorandom number generator.
 */
void GeneratorModule_unload(GeneratorModule *mod)
{
    mod->valid = 0;
    if (mod->lib != 0) {
        DLCLOSE_WRAPPER(mod->lib);
    }
}

///////////////////////////////////////
///// Hamming weights subroutines /////
///////////////////////////////////////


///////////////////////////////
///// Sorting subroutines /////
///////////////////////////////


/**
 * @brief 16-bit counting sort for 64-bit arrays.
 */
static void countsort64(uint64_t *out, const uint64_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = (size_t *) calloc(65536, sizeof(size_t));
    if (offsets == NULL) {
        fprintf(stderr, "***** countsort64: not enough memory *****\n");
        exit(1);
    }
    for (size_t i = 0; i < len; i++) {
        unsigned int pos = ((x[i] >> shr) & 0xFFFF);
        offsets[pos]++;
    }
    for (size_t i = 1; i < 65536; i++) {
        offsets[i] += offsets[i - 1];
    }
    for (size_t i = len; i-- != 0; ) {
        unsigned int digit = ((x[i] >> shr) & 0xFFFF);
        size_t offset = --offsets[digit];
        out[offset] = x[i];
    }
    free(offsets);
}


/**
 * @brief 16-bit counting sort for 32-bit arrays.
 */
static void countsort32(uint32_t *out, const uint32_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = calloc(65536, sizeof(size_t));
    if (offsets == NULL) {
        fprintf(stderr, "***** countsort32: not enough memory *****\n");
        exit(1);
    }
    for (size_t i = 0; i < len; i++) {
        unsigned int pos = ((x[i] >> shr) & 0xFFFF);
        offsets[pos]++;
    }
    for (size_t i = 1; i < 65536; i++) {
        offsets[i] += offsets[i - 1];
    }
    for (size_t i = len; i-- != 0; ) {
        unsigned int digit = ((x[i] >> shr) & 0xFFFF);
        size_t offset = --offsets[digit];
        out[offset] = x[i];
    }
    free(offsets);
}


/**
 * @brief Radix sort for 64-bit unsigned integers.
 */
void radixsort64(uint64_t *x, size_t len)
{
    uint64_t *out = calloc(len, sizeof(uint64_t));
    countsort64(out, x,   len, 0);
    countsort64(x,   out, len, 16);
    countsort64(out, x,   len, 32);
    countsort64(x,   out, len, 48);
    free(out);
}

/**
 * @brief Radix sort for 32-bit unsigned integers.
 */
void radixsort32(uint32_t *x, size_t len)
{
    uint32_t *out = calloc(len, sizeof(uint32_t));
    countsort32(out, x,   len, 0);
    countsort32(x,   out, len, 16);
    free(out);
}


/**
 * @brief Converts the number of seconds to the hours/minutes/seconds
 * format. Useful for showing the elapsed time.
 */
TimeHMS nseconds_to_hms(unsigned long nseconds_total)
{
    TimeHMS hms;
    hms.h = (nseconds_total / 3600);
    hms.m = (nseconds_total / 60) % 60;
    hms.s = nseconds_total % 60;
    return hms;
}

/**
 * @brief Prints elapsed time in hh:mm:ss format to stdout.
 * @param nseconds_total  Number of seconds.
 */
void print_elapsed_time(unsigned long nseconds_total)
{
    TimeHMS hms = nseconds_to_hms(nseconds_total);
    printf("%.2d:%.2d:%.2d", hms.h, hms.m, hms.s);
}


/////////////////////////////////////////////
///// TestsBattery class implementation /////
/////////////////////////////////////////////


size_t TestsBattery_ntests(const TestsBattery *obj)
{
    size_t ntests;
    for (ntests = 0; obj->tests[ntests].run != NULL; ntests++) { }
    return ntests;
}


#ifndef NOTHREADS

typedef struct {
#ifdef USE_PTHREADS
    pthread_t thrd_id;
#elif defined(USE_WINTHREADS)
    DWORD thrd_id;
    HANDLE thrd_handle;
#endif
    TestDescription *tests;
    size_t *tests_inds;
    size_t ntests;
    TestResults *results;
    const GeneratorInfo *gi;
    const CallerAPI *intf;
} BatteryThread;

#ifdef USE_WINTHREADS
static DWORD WINAPI battery_thread(void *data)
#else
static void *battery_thread(void *data)
#endif
{
    BatteryThread *th_data = data;
    th_data->intf->printf("vvvvvvvvvv Thread %lld started vvvvvvvvvv\n",
        get_thread_id(th_data->thrd_id));
    GeneratorState obj = GeneratorState_create(th_data->gi, th_data->intf);
    for (size_t i = 0; i < th_data->ntests; i++) {
        size_t ind = th_data->tests_inds[i];
        th_data->intf->printf("vvvvv Thread %lld: test %s started vvvvv\n",
            get_thread_id(th_data->thrd_id), th_data->tests[i].name);
        th_data->results[ind] = th_data->tests[i].run(&obj);
        th_data->intf->printf("^^^^^ Thread %lld: test %s finished ^^^^^\n",
            get_thread_id(th_data->thrd_id), th_data->tests[i].name);
        th_data->results[ind].name = th_data->tests[i].name;
        th_data->results[ind].thread_id = get_current_thread_id();
    }
    th_data->intf->printf("^^^^^^^^^^ Thread %lld finished ^^^^^^^^^^\n",
        get_thread_id(th_data->thrd_id));
    GeneratorState_free(&obj, th_data->intf);
    return 0;
}

typedef struct {
    size_t ind; ///< Text index in the array
    unsigned int nseconds; ///< Estimated time, seconds
} TestTiming;


static int cmp_TestTiming(const void *aptr, const void *bptr)
{
    const TestTiming *o1 = aptr, *o2 = bptr;
    if (o1->nseconds < o2->nseconds) { return 1; }
    else if (o1->nseconds == o2->nseconds) { return 0; }
    else { return -1; }
}


/**
 * @brief Sorts tests by the estimated execution time in the descending order.
 * It is used to distribute them between threads.
 */
static TestTiming *sort_tests_by_time(const TestDescription *descr, size_t ntests)
{
    TestTiming *out = calloc(ntests, sizeof(TestTiming));
    if (out == NULL) {
        fprintf(stderr, "sort_tests_by_time: out of memory");
        exit(1);
    }
    for (size_t i = 0; i < ntests; i++) {
        out[i].ind = i;
        out[i].nseconds = descr[i].nseconds;
    }
    qsort(out, ntests, sizeof(TestTiming), cmp_TestTiming);
    return out;
}

#endif
// NOTHREADS



static inline size_t test_pos_to_thread_ind(size_t test_pos, size_t nthreads)
{
    size_t thr_ind;
    if ((test_pos / nthreads) % 2 == 0) {
        thr_ind = test_pos % nthreads;
    } else {
        thr_ind = (nthreads - 1) - test_pos % nthreads;
    }
    return thr_ind;
}

/**
 * @brief Run the test battery in the multithreaded mode.
 */
static void TestsBattery_run_threads(const TestsBattery *bat, size_t ntests,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int nthreads, TestResults *results)
{
#ifndef NOTHREADS
    // Multithreaded version
    BatteryThread *th = calloc(nthreads, sizeof(BatteryThread));
    size_t *th_pos = calloc(nthreads, sizeof(size_t));
    // Preallocate arrays
    for (size_t i = 0; i < ntests; i++) {
        size_t ind = test_pos_to_thread_ind(i, nthreads);
        th[ind].ntests++;
    }
    for (size_t i = 0; i < nthreads; i++) {
        th[i].tests = calloc(th[i].ntests, sizeof(TestDescription));
        th[i].tests_inds = calloc(th[i].ntests, sizeof(size_t));
        th[i].gi = gen;
        th[i].intf = intf;
        th[i].results = results;
    }
    // Dispatch tests (and try to balance them between threads
    // according to estimations of elapsed time)
    // 0,1,...,(nthreads-1),(nthreads-1),...,2,1,0,0,1,2,....
    TestTiming *tt = sort_tests_by_time(bat->tests, ntests);
    for (size_t i = 0; i < ntests; i++) {
        size_t thr_ind = test_pos_to_thread_ind(i, nthreads);
        size_t test_ind = tt[i].ind;
        th[thr_ind].tests[th_pos[thr_ind]] = bat->tests[test_ind];
        th[thr_ind].tests_inds[th_pos[thr_ind]] = test_ind;
        th_pos[thr_ind]++;
    }
    free(tt);
    // Show balance
    for (size_t i = 0; i < nthreads; i++) {
        printf("Thread %d: ", (int) (i + 1));
        for (size_t j = 0; j < th[i].ntests; j++) {
            size_t test_ind = th[i].tests_inds[j];
            printf("%d(%s) ", (int) test_ind, bat->tests[test_ind].name);
        }
        printf("\n");
    }
#ifdef USE_PTHREADS
    // Run threads
    for (size_t i = 0; i < nthreads; i++) {
        pthread_create(&th[i].thrd_id, NULL, battery_thread, &th[i]);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        pthread_join(th[i].thrd_id, NULL);
    }
#elif defined(USE_WINTHREADS)
    // Run threads
    for (size_t i = 0; i < nthreads; i++) {
        th[i].thrd_handle = CreateThread(NULL, 0, battery_thread, &th[i], 0, &th[i].thrd_id);
    }
    // Get data from threads
    for (size_t i = 0; i < nthreads; i++) {
        WaitForSingleObject(th[i].thrd_handle, INFINITE);
    }
#endif
    // Deallocate array
    for (size_t i = 0; i < nthreads; i++) {
        free(th[i].tests);
        free(th[i].tests_inds);
    }
    free(th);
    free(th_pos);
#else
    (void) ntests;
    (void) nthreads;
    (void) results;
    printf("WARNING: multithreading is not supported on this platform\n");
    printf("Rerunning in one-threaded mode\n");
    TestsBattery_run(bat, gen, intf, TESTS_ALL, 1);
#endif
}


static void snprintf_pvalue(char *buf, size_t len, double p, double alpha)
{
    if (p != p || alpha != alpha) {
        snprintf(buf, len, "NAN");
    } else if (p < 0.0 || p > 1.0) {
        snprintf(buf, len, "???");
    } else if (p < DBL_MIN) {
        snprintf(buf, len, "0");
    } else if (1.0e-3 <= p && p <= 0.999) {
        snprintf(buf, len, "%.3f", p);
    } else if (p < 1.0e-3) {
        snprintf(buf, len, "%.2e", p);
    } else if (p > 0.999 && alpha > DBL_MIN) {
        snprintf(buf, len, "1 - %.2e", alpha);
    } else {
        snprintf(buf, len, "1");
    }
}

static void print_bar()
{
    for (int i = 0; i < 79; i++) {
        printf("-");
    }
    printf("\n");
}


static void TestResults_print_report(const TestResults *results,
    size_t ntests, time_t nseconds_total)
{
    unsigned int npassed = 0, nwarnings = 0, nfailed = 0;
    printf("  %3s %-20s %12s %14s %-15s %4s\n",
        "#", "Test name", "xemp", "p", "Interpretation", "Thr#");
    print_bar();
    for (size_t i = 0; i < ntests; i++) {
        char pvalue_txt[32];
        snprintf_pvalue(pvalue_txt, 32, results[i].p, results[i].alpha);
        printf("  %3d %-20s %12g %14s %-15s %4llu\n",
            (int) i + 1, results[i].name, results[i].x, pvalue_txt,
            interpret_pvalue(results[i].p),
            (unsigned long long) results[i].thread_id);
        switch (get_pvalue_category(results[i].p)) {
        case pvalue_passed:
            npassed++; break;
        case pvalue_warning:
            nwarnings++; break;
        case pvalue_failed:
            nfailed++; break;
        }
    }
    print_bar();
    printf("Passed:       %u\n", npassed);
    printf("Suspicious:   %u\n", nwarnings);
    printf("Failed:       %u\n", nfailed);
    printf("Elapsed time: "); print_elapsed_time(nseconds_total);
    printf("\n\n");
}

/**
 * @brief Prints a summary about the test battery to stdout.
 */
void TestsBattery_print_info(const TestsBattery *obj)
{
    size_t ntests = TestsBattery_ntests(obj);
    unsigned long nseconds_total = 0;
    printf("===== Battery '%s' summary =====\n", obj->name);
    printf("  %3s %-20s %-20s\n",
        "#", "Test name", "Estimated time, sec");
    print_bar();
    for (size_t i = 0; i < ntests; i++) {
        printf("  %3d %-20s %6u\n",
            (int) i + 1, obj->tests[i].name, obj->tests[i].nseconds);
        nseconds_total += obj->tests[i].nseconds;
    }
    print_bar();
    printf("Estimated elapsed time (one-threaded): ");
    print_elapsed_time(nseconds_total);
    printf("\n\n");
}

/**
 * @brief Runs the given battery of the statistical test for the given
 * pseudorandom number generator.
 */
void TestsBattery_run(const TestsBattery *bat,
    const GeneratorInfo *gen, const CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    time_t tic, toc;
    size_t ntests = TestsBattery_ntests(bat);
    size_t nresults = ntests;
    TestResults *results = NULL;
#ifdef NOTHREADS
    nthreads = 1;
#endif
    printf("===== Starting '%s' battery =====\n", bat->name);
    if (testid > ntests) { // testid is 1-based index
        fprintf(stderr, "Invalid test id %u", testid);
        return;
    }
    // Allocate memory for tests
    if (testid == TESTS_ALL) {
        results = calloc(ntests, sizeof(TestResults));
        nresults = ntests;
    } else {
        results = calloc(1, sizeof(TestResults));
        nresults = 1;
    }
    if (results == NULL) {
        fprintf(stderr, "***** TestsBattery_run: not enough memory *****\n");
        exit(1);
    }
    // Run the tests
    tic = time(NULL);
    if (nthreads == 1 || testid != TESTS_ALL) {
        // One-threaded version
        GeneratorState obj = GeneratorState_create(gen, intf);
        if (testid == TESTS_ALL) {
            for (size_t i = 0; i < ntests; i++) {
                results[i] = bat->tests[i].run(&obj);
                results[i].name = bat->tests[i].name;
                results[i].thread_id = 0;
            }
        } else {
            *results = bat->tests[testid - 1].run(&obj);
            results->name = bat->tests[testid - 1].name;
        }
        GeneratorState_free(&obj, intf);
    } else {
       // Multithreaded version
       TestsBattery_run_threads(bat, ntests, gen, intf, nthreads, results);
    }
    toc = time(NULL);
    printf("\n");
    printf("==================== Seeds logger report ====================\n");
    Entropy_print_seeds_log(&entropy, stdout);
    if (testid == TESTS_ALL) {
        printf("==================== '%s' battery report ====================\n", bat->name);
    } else {
        printf("==================== '%s' battery test #%d report ====================\n",
            bat->name, testid);
    }
    printf("Generator name:    %s\n", gen->name);
    printf("Output size, bits: %d\n\n", (int) gen->nbits);
    TestResults_print_report(results, nresults, toc - tic);
    free(results);
}


/**
 * @brief Keeps the original generator for application of different filters to it.
 */
static GeneratorInfo original_gen;

/**
 * @brief Buffer for decomposition of each 64-bit value into a pair
 * of 32-bit values. Needed for TestU01: its batteries should have
 * an access to all bits. Used by MAKE_UINT64_INTERLEAVED_PRNG macro.
 */
typedef struct {
    union {
        uint64_t u64; ///< To be splitted to two 32-bit values.
        uint32_t u32[2]; ///< 32-bit values.
    } val; ///< Internal buffer for 32-bit outputs.
    int pos; ///< Output position for 32-bit output.
} Interleaved32Buffer;


Interleaved32Buffer interleaved_buf;

////////////////////////////////////////////////////////////////
///// Implementation of generator with reversed bits order /////
////////////////////////////////////////////////////////////////

static uint64_t get_bits32_reversed(void *state)
{
    return reverse_bits32((uint32_t) original_gen.get_bits(state));
}

static uint64_t get_bits64_reversed(void *state)
{
    return reverse_bits64(original_gen.get_bits(state));
}

GeneratorInfo reversed_generator_set(const GeneratorInfo *gi)
{
    GeneratorInfo reversed_gen = *gi;
    if (gi->nbits == 32) {
        reversed_gen.get_bits = get_bits32_reversed;
    } else {
        reversed_gen.get_bits = get_bits64_reversed;
    }
    original_gen = *gi;
    return reversed_gen;
}


///////////////////////////////////////////////////////////////////
///// Implementation of generator with interleaved bits order /////
///////////////////////////////////////////////////////////////////

static uint64_t get_bits32_interleaved(void *state)
{
    if (interleaved_buf.pos == 2) {
        interleaved_buf.val.u64 = original_gen.get_bits(state);
        interleaved_buf.pos = 0;
    }
    return interleaved_buf.val.u32[interleaved_buf.pos++];
}

GeneratorInfo interleaved_generator_set(const GeneratorInfo *gi)
{
    GeneratorInfo interleaved_gen = *gi;
    interleaved_gen.get_bits = get_bits32_interleaved;
    original_gen = *gi;
    return interleaved_gen;
}


///////////////////////////////////////////////
///// Implementation of file input/output /////
///////////////////////////////////////////////


/**
 * @brief Switches stdout to binary mode in MS Windows (needed for
 * correct output of binary data)
 */
void set_bin_stdout()
{
#ifdef USE_LOADLIBRARY
    (void) _setmode( _fileno(stdout), _O_BINARY);
#endif
}


/**
 * @brief Switches stdin to binary mode in MS Windows (needed for
 * correct input of binary data)
 */
void set_bin_stdin()
{
#ifdef USE_LOADLIBRARY
    (void) _setmode( _fileno(stdin), _O_BINARY);
#endif
}


/**
 * @brief Dump an output PRNG to the stdout in the format suitable
 * for PractRand.
 */
void GeneratorInfo_bits_to_file(GeneratorInfo *gen, const CallerAPI *intf)
{
    set_bin_stdout();
    void *state = gen->create(intf);
    if (gen->nbits == 32) {
        uint32_t buf[256];
        while (1) {
            for (size_t i = 0; i < 256; i++) {
                buf[i] = gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint32_t), 256, stdout);
        }
    } else {
        uint64_t buf[256];
        while (1) {
            for (size_t i = 0; i < 256; i++) {
                buf[i] = gen->get_bits(state);
            }
            fwrite(buf, sizeof(uint64_t), 256, stdout);
        }
    }
}
