#include "smokerand/smokerand_core.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static uint32_t get_seed32(void)
{
    return time(NULL);
}

static uint64_t get_seed64(void)
{
    return time(NULL);
}

CallerAPI CallerAPI_init()
{
    CallerAPI intf;
    intf.get_seed32 = get_seed32;
    intf.get_seed64 = get_seed64;
    intf.malloc = malloc;
    intf.free = free;
    intf.printf = printf;
    intf.strcmp = strcmp;
    return intf;
}



/////////////////////////////////////// 
///// Some mathematical functions /////
///////////////////////////////////////


/**
 * @brief A crude approximation for Kolmogorov-Smirnov test p-values.
 * @details Control (x, p) points from scipy.special.kolmogorov:
 * - (5.0, 3.8575e-22), (2.0, 0.0006709), (1.1, 0.1777),
 * - (1.0, 0.2700), (0.9, 0.3927)
 */
double ks_pvalue(double x)
{
    double xsq = x * x;
    if (x <= 0.0) {
        return 1.0;
    } else if (x > 1.0) {
        return 2.0 * (exp(-2.0 * xsq) - exp(-8.0 * xsq));
    } else {
        double t = -M_PI * M_PI / (8 * xsq);
        return 1.0 - sqrt(2 * M_PI) / x * (exp(t) + exp(9*t));
    }
}


/**
 * @brief An approximation of incomplete gamma function for implementation
 * of Poisson C.D.F. and its p-values computation.
 * @details References:
 * - https://gmd.copernicus.org/articles/3/329/2010/gmd-3-329-2010.pdf
 * - https://functions.wolfram.com/GammaBetaErf/Gamma2/06/02/02/
 */
double gammainc(double a, double x)
{
    double mul = exp(-x + a*log(x) - lgamma(a)), sum = 0.0;
    if (x < 10 * a) {
        double t = 1.0 / a;
        for (int i = 1; i < 1000 && t > DBL_EPSILON; i++) {
            sum += t;
            t *= x / (a + i);
        }
        return mul * sum;
    } else {
        sum = 1 / x * ( 1 - (1 - a) / x - (2 - a) * (1 - a) / (x*x));
        return 1.0 - mul * sum;
    }
}

/**
 * @brief Poisson distribution C.D.F. (cumulative distribution function)
 */
double poisson_cdf(double x, double lambda)
{
    return 1.0 - gammainc(floor(x) + 1.0, lambda);
}

/**
 * @brief Calculate p-value for Poission distribution as 1 - F(x)
 * where F(x) is Poission distribution C.D.F.
 */
double poisson_pvalue(double x, double lambda)
{
    return gammainc(floor(x) + 1.0, lambda);
}


/**
 * @brief Asymptotic approximation for p-value of chi-square distribution.
 * @details References:
 * - Wilson E.B., Hilferty M.M. The distribution of chi-square // Proceedings
 * of the National Academy of Sciences. 1931. Vol. 17. N 12. P. 684-688.
 * https://doi.org/10.1073/pnas.17.12.684
 */
double chi2_pvalue(double x, unsigned long f)
{
    double s2 = 2.0 / (9.0 * f);
    double mu = 1 - s2;
    double z = (pow(x/f, 1.0/3.0) - mu) / sqrt(s2);
    return 0.5 * erfc(z / sqrt(2));
}

//////////////////////////////////////////////////
///// Subroutines for working with C modules /////
//////////////////////////////////////////////////


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

    GetGenInfoFunc gen_getinfo = (void *) DLSYM_WRAPPER(mod.lib, "gen_getinfo");
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        mod.valid = 0;
    }
    if (!gen_getinfo(&mod.gen)) {
        fprintf(stderr, "'gen_getinfo' function failed\n");
        mod.valid = 0;
    }

    return mod;
}


void GeneratorModule_unload(GeneratorModule *mod)
{
    mod->valid = 0;
    if (mod->lib != 0) {
        DLCLOSE_WRAPPER(mod->lib);
    }
}


///////////////////////////////
///// Sorting subroutines /////
///////////////////////////////


/**
 * @brief 16-bit counting sort for 64-bit arrays.
 */
static void countsort64(uint64_t *out, const uint64_t *x, size_t len, unsigned int shr)
{
    size_t *offsets = (size_t *) calloc(65536, sizeof(size_t));
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
    uint64_t *out = (uint64_t *) calloc(len, sizeof(uint64_t));
    countsort64(out, x,   len, 0);
    countsort64(x,   out, len, 16);
    countsort64(out, x,   len, 32);
    countsort64(x,   out, len, 48);
    free(out);
}

/**
 * @brief Radix sort for 32-bit unsigned integers.
 */
void radixsort32(uint64_t *x, size_t len)
{
    uint64_t *out = (uint64_t *) calloc(len, sizeof(uint64_t));
    countsort64(out, x,   len, 0);
    countsort64(x,   out, len, 16);
    free(out);
}

/////////////////////////////
///// Statistical tests /////
/////////////////////////////


static unsigned int bspace_get_ndups(uint64_t *x, size_t len, int nbits)
{
    unsigned int ndups = 0;
    if (nbits == 32) {
        radixsort32(x, len);
    } else {
        radixsort64(x, len);
    }
    for (size_t i = 0; i < len - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    if (nbits == 32) {
        radixsort32(x, len - 1);
    } else {
        radixsort64(x, len - 1);
    }
    for (size_t i = 0; i < len - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}


/**
 * @brief n-dimensional birthday spacings test.
 */
TestResults bspace_nd_test(BSpaceNDOptions *opts, const GeneratorInfo *gi, void *state,
    const CallerAPI *intf)
{
    TestResults ans;
    unsigned int genout_nbits = GeneratorInfo_get_nbits(gi);
    if (opts->ndims * opts->nbits_per_dim > 64 || genout_nbits == 0) {
        return ans;
    }
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t mask = (1ull << opts->nbits_per_dim) - 1ull;
    printf("%u %llX\n", nbits_total, mask);
    size_t len = pow(2.0, (nbits_total + 4.0) / 3.0);
    double lambda = pow(len, 3.0) / (4 * pow(2.0, nbits_total));
    size_t nsamples = (1ull << 24) / len;
    if (nsamples < 5) nsamples = 5;
    // Show information about the test
    intf->printf("Birthday spacings test\n");
    intf->printf("  ndims = %d; nbits_per_dim = %d; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    intf->printf("  nsamples = %lld; len = %lld, lambda = %g\n",
        nsamples, len, lambda);
    // Compute number of duplicates
    uint64_t *u = calloc(len, sizeof(uint64_t));
    uint64_t *ndups = calloc(nsamples, sizeof(uint64_t));
    ans.name = "Birthday spacings (ND)";
    for (size_t i = 0; i < nsamples; i++) {
        if (genout_nbits == 64) {
            for (size_t j = 0; j < len; j++) {
                u[j] = 0;
                for (size_t k = 0; k < opts->ndims; k++) {
                    u[j] <<= opts->nbits_per_dim;
                    u[j] |= gi->get_bits64(state) & mask;
                }
            }
        } else {
            for (size_t j = 0; j < len; j++) {
                u[j] = 0;
                for (size_t k = 0; k < opts->ndims; k++) {
                    u[j] <<= opts->nbits_per_dim;
                    u[j] |= gi->get_bits32(state) & mask;
                }
            }
        }
        if (nbits_total > 32) {
            ndups[i] = bspace_get_ndups(u, len, 64);
        } else {
            ndups[i] = bspace_get_ndups(u, len, 32);
        }
    }    
    // Statistical analysis
    if (nsamples < 512) {
        // Variant a: total number of duplicates
        intf->printf("  Analysis of total number of duplicates (Poisson distribution)\n");
        uint64_t ndups_total = 0;
        for (size_t i = 0; i < nsamples; i++) {
            ndups_total += ndups[i];
        }
        ans.x = (double) ndups_total;
        ans.p = poisson_cdf(ans.x, lambda * nsamples);
    } else {
        // Variant b: chi square criteria
        intf->printf("  Analysis of discrete distribution (chi-square distribution)\n");
        size_t nbins = lambda * 4;
        unsigned int *Oi = calloc(nbins, sizeof(unsigned int));
        unsigned int Oi_sum = 0;
        for (size_t i = 0; i < nsamples; i++) {
            size_t ind = (ndups[i] < nbins) ? ndups[i] : nbins;
            Oi[ind]++;
            Oi_sum++;
        }
        printf("%6s", "#");
        for (size_t i = 0; i < nbins; i++) {
            if (i != nbins - 1) {
                printf("%5d ", (int) i);
            } else {
                printf(">=%3d ", (int) i);
            }
        }
        printf("\n%6s", "Oi:");
        for (size_t i = 0; i < nbins; i++) {
            printf("%5d ", Oi[i]);
        }
        double Ei = exp(-lambda) * nsamples;
        ans.x = 0.0; // chi2emp
        printf("\n%6s", "Ei:");
        for (size_t i = 0; i < nbins; i++) {
            printf("%5.0f ", Ei);
            ans.x += pow(Oi[i] - Ei, 2.0) / Ei;
            Ei *= lambda / (i + 1.0);
        }
        printf("\n");
        ans.p = chi2_pvalue(ans.x, nbins - 1);
        free(Oi);
    }

    // Kolmogorov-Smirnov test
/*
    radixsort64(ndups, nsamples);
    double D = 0.0;
    for (size_t i = 0; i < nsamples; i++) {
        double f = poisson_cdf(ndups[i], lambda);
        double idbl = (double) i;        
        double Dplus = (idbl + 1.0) / nsamples - f;
        double Dminus = f - idbl / nsamples;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
        //printf("%g %g %g | %g %d\n", f, Dplus, Dminus, lambda, (int) ndups[i]);
    }
    ans.x = sqrt(nsamples) * D + 1.0 / (6.0 * sqrt(nsamples));
    ans.p = ks_pvalue(ans.x);
*/
    
    free(u);
    free(ndups);
    return ans;
}
