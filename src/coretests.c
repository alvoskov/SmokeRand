/**
 * @file coretests.c
 * @brief The key tests for testings PRNGs: frequency tests, Marsaglia's
 * birthday spacings and monkey tests, Knuth's gap test.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/coretests.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/////////////////////////////
///// Statistical tests /////
/////////////////////////////


/**
 * @brief Make non-overlapping 64-bit tuples (points in n-dimensional space)
 * for birthday spacings test. It may use either higher or lower bits.
 */
static inline void bspace_make_tuples64(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint64_t *u, size_t len)
{
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 64) {
            mask = 0xFFFFFFFFFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) & mask;
            }
        }
    } else {
        // Take higher bits
        unsigned int shl = gi->nbits - opts->nbits_per_dim;
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) >> shl;
            }
        }
    }
}


/**
 * @brief Make non-overlapping 32-bit tuples (points in n-dimensional space)
 * for birthday spacings test. It may use either higher or lower bits.
 */
static inline void bspace_make_tuples32(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint32_t *u, size_t len)
{
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 32) {
            mask = 0xFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) & mask;
            }
        }
    } else {
        // Take higher bits
        unsigned int shl = gi->nbits - opts->nbits_per_dim;
        for (size_t j = 0; j < len; j++) {
            u[j] = 0;
            for (size_t k = 0; k < opts->ndims; k++) {
                u[j] <<= opts->nbits_per_dim;
                u[j] |= gi->get_bits(state) >> shl;
            }
        }
    }
}


static unsigned int bspace_get_ndups64(uint64_t *x, size_t len)
{
    unsigned int ndups = 0;
    radixsort64(x, len);
    for (size_t i = 0; i < len - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    radixsort64(x, len - 1);
    for (size_t i = 0; i < len - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}


static unsigned int bspace_get_ndups32(uint32_t *x, size_t len)
{
    unsigned int ndups = 0;
    radixsort32(x, len);
    for (size_t i = 0; i < len - 1; i++) {
        x[i] = x[i + 1] - x[i];
    }
    radixsort32(x, len - 1);
    for (size_t i = 0; i < len - 2; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    return ndups;
}



/**
 * @brief 32-bit version of n-dimensional birthday spacings test.
 * @return Number of duplicates.
 */
static unsigned long bspace32_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = pow(2.0, (nbits_total + 4.0) / 3.0);
    uint32_t *u = calloc(len, sizeof(uint32_t));
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    for (size_t i = 0; i < opts->nsamples; i++) {
        bspace_make_tuples32(opts, obj->gi, obj->state, u, len);
            ndups[i] = bspace_get_ndups32(u, len);
    }
    unsigned long ndups_total = 0;
    for (size_t i = 0; i < opts->nsamples; i++) {
        ndups_total += ndups[i];
    }
    free(u);
    free(ndups);
    return ndups_total;
}

/**
 * @brief 64-bit version of n-dimensional birthday spacings test.
 * @return Number of duplicates.
 */
static size_t bspace64_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = pow(2.0, (nbits_total + 4.0) / 3.0);
    uint64_t *u = calloc(len, sizeof(uint64_t));
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    for (size_t i = 0; i < opts->nsamples; i++) {
        bspace_make_tuples64(opts, obj->gi, obj->state, u, len);
            ndups[i] = bspace_get_ndups64(u, len);
    }
    unsigned long ndups_total = 0;
    for (size_t i = 0; i < opts->nsamples; i++) {
        ndups_total += ndups[i];
    }
    free(u);
    free(ndups);
    return ndups_total;
}



/**
 * @brief n-dimensional birthday spacings test.
 */
TestResults bspace_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    TestResults ans;
    if (opts->ndims * opts->nbits_per_dim > 64 ||
        (obj->gi->nbits != 32 && obj->gi->nbits != 64)) {
        return ans;
    }
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = pow(2.0, (nbits_total + 4.0) / 3.0);
    double lambda = pow(len, 3.0) / (4 * pow(2.0, nbits_total));
    // Show information about the test
    obj->intf->printf("Birthday spacings test\n");
    obj->intf->printf("  ndims = %d; nbits_per_dim = %d; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lld; len = %lld, lambda = %g\n",
        opts->nsamples, len, lambda);
    // Compute number of duplicates
    unsigned long ndups_total = 0;
    if (nbits_total > 32) {
        ndups_total = bspace64_nd_test(obj, opts);
    } else {
        ndups_total = bspace32_nd_test(obj, opts);
    }
    ans.x = (double) ndups_total;
    ans.p = poisson_pvalue(ans.x, lambda * opts->nsamples);
    ans.alpha = poisson_cdf(ans.x, lambda * opts->nsamples);
    obj->intf->printf("  x = %.0f; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}


/**
 * @brief Make overlapping tuples (points in n-dimensional space) for
 * collisionover test. It may use either higher or lower bits.
 */
static inline void collisionover_make_tuples(const BSpaceNDOptions *opts,
    const GeneratorInfo *gi, void *state, uint64_t *u, size_t len)
{
    uint64_t cur_tuple = 0;
    const int rshift = (opts->ndims - 1) * opts->nbits_per_dim;
    if (opts->get_lower) {
        // Take lower bits
        uint64_t mask;
        if (opts->nbits_per_dim == 64) {
            mask = 0xFFFFFFFFFFFFFFFF;
        } else {
            mask = (1ull << opts->nbits_per_dim) - 1ull;
        }
        // Initialize the first tuple
        for (int j = 0; j < 8; j++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) & mask) << rshift;
        }
        // Generate other tuples
        for (size_t i = 0; i < len; i++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) & mask) << rshift;
            u[i] = cur_tuple;
        }
    } else {
        // Take higher bits
        unsigned int shr = gi->nbits - opts->nbits_per_dim;
        // Initialize the first tuple
        for (int j = 0; j < 8; j++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) >> shr) << rshift;
        }
        // Generate other tuples
        for (size_t i = 0; i < len; i++) {
            cur_tuple >>= opts->nbits_per_dim;
            cur_tuple |= (gi->get_bits(state) >> shr) << rshift;
            u[i] = cur_tuple;
        }
    }
}


/**
 * @brief "CollisionOver" modification of "Monkey test". The algorithm was
 * suggested by developers of TestU01.
 */
TestResults collisionover_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    size_t n = 50000000;
    uint64_t *u = calloc(n, sizeof(uint64_t));
    uint64_t Oi[4] = {1ull << opts->ndims * opts->nbits_per_dim, 0, 0, 0};
    double nstates = Oi[0];
    double lambda = (n - opts->ndims + 1.0) / nstates;
    double mu = nstates * (lambda - 1 + exp(-lambda));
    TestResults ans;
    ans.name = "CollisionOver";
    obj->intf->printf("CollisionOver test\n");
    obj->intf->printf("  ndims = %d; nbits_per_dim = %d; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lld; len = %lld, mu = %g\n",
        opts->nsamples, n, mu);

    ans.x = 0;
    for (size_t i = 0; i < opts->nsamples; i++) {
        collisionover_make_tuples(opts, obj->gi, obj->state, u, n);
        // Find collisions by sorting the array
        radixsort64(u, n);
        size_t ncopies = 0;
        for (size_t i = 0; i < n - 1; i++) {
            if (u[i] == u[i + 1]) {
                ncopies++;
            } else {
                Oi[(ncopies < 3) ? (ncopies + 1) : 3]++;
                Oi[0]--;
                ncopies = 0;
            }
        }
    }
    ans.x += Oi[2];
    ans.p = poisson_pvalue(ans.x, mu * opts->nsamples);
    ans.alpha = poisson_cdf(ans.x, mu * opts->nsamples);
    // Frequency table
    double Ei = exp(-lambda) * nstates * opts->nsamples;
    obj->intf->printf("  %5s %16s %16s\n", "Freq", "Oi", "Ei");
    for (int i = 0; i < 4; i++) {
        obj->intf->printf("  %5d %16lld %16.1f\n", i, Oi[i], Ei);
        Ei *= lambda / (i + 1.0);
    }
    // Calculate statistics for p-value
    obj->intf->printf("  lambda = %g, mu = %g * %d\n",
        lambda, mu, (int) opts->nsamples);
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    free(u);
    return ans;
}

///////////////////////////////////
///// Gap test implementation /////
///////////////////////////////////

/**
 * @brief Knuth's gap test for detecting lagged Fibonacci generators.
 * @details Gap is \f$ [0;\beta) \f$ where $\beta = 1 / (2^{shl}) \f$.
 */
TestResults gap_test(GeneratorState *obj, unsigned int shl)
{
    const double Ei_min = 10.0;
    double p = 1.0 / (1 << shl); // beta in the floating point format
    uint64_t beta = 1ull << (obj->gi->nbits - shl);
    uint64_t u;
    size_t ngaps = 10000000;
    size_t nbins = log(Ei_min / (ngaps * p)) / log(1 - p);
    size_t *Oi = calloc(nbins + 1, sizeof(size_t));
    TestResults ans;
    ans.name = "Gap";
    obj->intf->printf("Gap test\n");
    obj->intf->printf("  alpha = 0.0; beta = %g; shl = %u;\n", p, shl);
    obj->intf->printf("  ngaps = %llu; nbins = %llu\n", ngaps, nbins);
    for (size_t i = 0; i < ngaps; i++) {
        size_t gap_len = 0;
        u = obj->gi->get_bits(obj->state);
        while (u > beta) {
            gap_len++;
            u = obj->gi->get_bits(obj->state);
        }
        Oi[(gap_len < nbins) ? gap_len : nbins]++;
    }
    ans.x = 0.0; // chi2emp
    for (size_t i = 0; i < nbins; i++) {
        double Ei = p * pow(1.0 - p, i) * ngaps;
        double d = Ei - Oi[i];
        ans.x += d * d / Ei;
    }
    free(Oi);
    ans.p = chi2_pvalue(ans.x, nbins - 1);
    ans.alpha = chi2_cdf(ans.x, nbins - 1);
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}


/**
 * @brief Monobit frequency test.
 */
TestResults monobit_freq_test(GeneratorState *obj)
{
    int sum_per_byte[256];
    size_t len = 1ull << 28;
    TestResults ans;
    ans.name = "MonobitFreq";
    for (size_t i = 0; i < 256; i++) {
        uint8_t u = i;
        sum_per_byte[i] = 0;
        for (size_t j = 0; j < 8; j++) {
            if ((u & 0x1) != 0) {
                sum_per_byte[i]++;
            }
            u >>= 1;
        }
        sum_per_byte[i] -= 4;
    }
    int64_t bitsum = 0;
    unsigned int nbytes = obj->gi->nbits / 8;
    for (size_t i = 0; i < len; i++) {
        uint64_t u = obj->gi->get_bits(obj->state);
        for (size_t j = 0; j < nbytes; j++) {
            bitsum += sum_per_byte[u & 0xFF];
            u >>= 8;
        }
    }
    ans.x = fabs((double) bitsum) / sqrt(len * obj->gi->nbits);
    ans.p = erfc(ans.x / sqrt(2));
    ans.alpha = erf(ans.x / sqrt(2));
    obj->intf->printf("Monobit frequency test\n");
    obj->intf->printf("  Number of bits: %llu\n", len * obj->gi->nbits);
    obj->intf->printf("  sum = %lld; x = %g; p = %g\n",
        bitsum, ans.x, ans.p);
    obj->intf->printf("\n");    
    return ans;
}


static int cmp_doubles(const void *aptr, const void *bptr)
{
    double aval = *((double *) aptr), bval = *((double *) bptr);
    if (aval < bval) return -1;
    else if (aval == bval) return 0;
    else return 1;
}

/**
 * @brief Bytes frequency test.
 */
TestResults byte_freq_test(GeneratorState *obj)
{
    size_t block_len = 1ull << 16;
    size_t nblocks = 4096;
    size_t bytefreq[256];
    
    unsigned int nbytes_per_num = obj->gi->nbits / 8;
    unsigned int nbytes = nbytes_per_num * block_len;
    double *chi2 = calloc(nblocks, sizeof(double));

    TestResults ans;
    ans.name = "ByteFreq";
    obj->intf->printf("Byte frequency test\n");
    obj->intf->printf("  nblocks = %lld; block_len = %lld\n",
            (unsigned long long) nblocks,
            (unsigned long long) block_len);
    for (size_t ii = 0; ii < nblocks; ii++) {
        for (size_t i = 0; i < 256; i++) {
            bytefreq[i] = 0;
        }
        for (size_t i = 0; i < block_len; i++) {
            uint64_t u = obj->gi->get_bits(obj->state);        
            for (size_t j = 0; j < nbytes_per_num; j++) {
                bytefreq[u & 0xFF]++;
                u >>= 8;
            }
        }
        chi2[ii] = 0;
        double Ei = nbytes / 256.0;
        for (size_t i = 0; i < 256; i++) {
            chi2[ii] += pow(bytefreq[i] - Ei, 2.0) / Ei;
        }
    }
    // Kolmogorov-Smirnov test
    qsort(chi2, nblocks, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < nblocks; i++) {
        double f = chi2_cdf(chi2[i], 255);
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / nblocks - f;
        double Dminus = f - idbl / nblocks;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double K = sqrt(nblocks) * D + 1.0 / (6.0 * sqrt(nblocks));
    ans.x = K;
    ans.p = ks_pvalue(K);
    ans.alpha = 1.0 - ans.p;
    obj->intf->printf("  K = %g, p = %g\n", K, ans.p);
    obj->intf->printf("\n");
    free(chi2);
    return ans;
}

