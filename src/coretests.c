/**
 * @file coretests.c
 * @brief The key tests for testings PRNGs: frequency tests, Marsaglia's
 * birthday spacings and monkey tests, Knuth's gap test. Also includes
 * the mod3 test from gjrand.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/coretests.h"
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/////////////////////////////////////////////////
///// Birthday spacings test implementation /////
/////////////////////////////////////////////////


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

static unsigned long bspace_calc_len(unsigned int nbits_total)
{
    return (unsigned long) pow(2.0, (nbits_total + 4.0) / 3.0);
}

static double bspace_calc_lambda(size_t len, unsigned int nbits_total)
{
    return pow((double) len, 3.0) / (4 * pow(2.0, nbits_total));
}

/**
 * @brief 32-bit version of n-dimensional birthday spacings test.
 * @return Number of duplicates.
 */
static unsigned long bspace32_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = bspace_calc_len(nbits_total);
    uint32_t *u = calloc(len, sizeof(uint32_t));
    if (u == NULL) {
        fprintf(stderr, "***** bspace32_nd_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    if (ndups == NULL) {
        fprintf(stderr, "***** bspace32_nd_test: not enough memory *****\n");
        exit(1);
    }
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
static unsigned long bspace64_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    size_t len = bspace_calc_len(nbits_total);
    uint64_t *u = calloc(len, sizeof(uint64_t));
    if (u == NULL) {
        fprintf(stderr, "***** bspace64_nd_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long *ndups = calloc(opts->nsamples, sizeof(unsigned long));
    if (ndups == NULL) {
        fprintf(stderr, "***** bspace64_nd_test: not enough memory *****\n");
        exit(1);
    }
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
 * @details An implementation of classical N-dimensional birthday spacings test
 * suggested by G. Marsaglia. The number of duplicates in spacings obeys
 * Poisson distribution with the next mathematical expectance:
 *
 * \f[
 * \lambda = \frac{n^3}{4m}
 * \f]
 *
 * where \f$n\f$ is the number of points in the sample and \f$m\f$ is the
 * number of "birthdays" (number of possible values per point, if each point
 * is encoded by \f$ k \f$ bits - then \f$ m = 2^k \f$.
 *
 * The \f$n\f$ value is automatically selected to provide \f$ \lambda = 4\f$,
 * an optimal parameter for Poisson distribution.
 *
 * In the case of 1-dimensional test one point is made from 1 pseudorandom
 * value, in the case of N-dimensional test -- from lower or higher bits of 
 * N points. These tuples are NOT OVERLAPPING!
 *
 * References:
 *
 * 1. Marsaglia G., Tsang W. W. Some Difficult-to-pass Tests of Randomness
 *    // Journal of Statistical Software. 2002. V. 7. N. 3. P.1-9.
 *    https://doi.org/10.18637/jss.v007.i03
 */
TestResults bspace_nd_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    TestResults ans = TestResults_create("bspace_nd");
    if (opts->ndims * opts->nbits_per_dim > 64 ||
        (obj->gi->nbits != 32 && obj->gi->nbits != 64)) {
        return ans;
    }
    unsigned int nbits_total = opts->ndims * opts->nbits_per_dim;
    unsigned long len = bspace_calc_len(nbits_total);
    double lambda = bspace_calc_lambda(len, nbits_total);
    // Show information about the test
    obj->intf->printf("Birthday spacings test\n");
    obj->intf->printf("  ndims = %u; nbits_per_dim = %u; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lu; len = %lu, lambda = %g\n",
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

static uint64_t get_bits64_from32(void *state)
{
    GeneratorState *obj = state;
    uint64_t x = obj->gi->get_bits(obj->state);
    uint64_t y = obj->gi->get_bits(obj->state);
    return (x << 32) | y;
}


/**
 * @brief One-dimensional 32-bit birthday spacings test.
 * In the case of 32-bit PRNG it automatically switches to the 2D 32-bit
 * birthday spacings test.
 */
TestResults bspace64_1d_ns_test(GeneratorState *obj, unsigned int nsamples)
{
    BSpaceNDOptions opts;
    opts.nbits_per_dim = 64;
    opts.ndims = 1;
    opts.nsamples = nsamples;
    opts.get_lower = 1;
    if (obj->gi->nbits == 64) {
        return bspace_nd_test(obj, &opts);
    } else {
        GeneratorInfo genenv_info = *(obj->gi);
        GeneratorState genenv;
        genenv_info.get_bits = get_bits64_from32;
        genenv.gi = &genenv_info;
        genenv.state = obj; // Enveloped generator
        genenv.intf = obj->intf;
        return bspace_nd_test(&genenv, &opts);
    }
}


static void bspace4_8d_decimated_pvalue(TestResults *ans, const char *name,
    uint32_t *u, size_t len, double lambda, const CallerAPI *intf)
{
    double x = (double) bspace_get_ndups32(u, len);
    double p = poisson_pvalue(x, lambda);
    double alpha = poisson_cdf(x, lambda);
    intf->printf("  %-30s x = %6.0f; p = %g\n", name, x, p);
    if (p < ans->p && (p < 1e-10 || ans->p > 1.0)) {
        ans->p = p;
        ans->x = x;
        ans->alpha = alpha;
    }
}


/**
 * @brief A specialized version of birthday spacings test designed for
 * detection of truncated 128-bit and 96-bit LCGs with modulo \f$ 2^k \f$.
 * @details It is a modification of 8-dimensional 8-bit (lower bits) birthday
 * spacings test. But only every 1 of `step` values are used for construction
 * of tuples. The `step` value should be power of 2. Such decimation reduces
 * an effective period of LCG and allows to detect anomalies in 96-bit
 * and 128-bit LCGs with module \f$ 2^{96} \f$ or \f$ 2^{128} \f$. Not
 * sensitive to LCGs with prime modulus.
 *
 * An idea of the test was partially inspired by the TMFn test from
 * PractRand 0.94 test suite by Chris Doty-Humphrey. The TMFn test also uses
 * decimation and takes only 1 or 2 64-bit values at the beginning of blocks
 * consisting of 4096 bytes. But they are analyzed by some mysterous bitwise
 * operations and g-test, not by means of birthday spacings.
 *
 * Sensitivity of 64-bit version of decimated birthday spacings test
 * for different steps:
 *
 * - 256 is enough to detect 128-bit LCG with truncation of lower 64 bits
 * - 8192 is enough to detect the same PRNG with truncation of lower 96 bits.
 *
 * Sensitivity of 32-bit version for different steps:
 *
 * - \f$2^{6}\f$ (64): detects 64-bit LCG with truncation of lower 32 bits.
 * - \f$2^{12}\f$ (4096): detects 128-bit LCG with truncation of lower 64 bits.
 * - \f$2^{18}\f$ (262144) detects 128-bit LCG with truncation of lower 96 bits.
 * - \f$2^{24}\f$: upper 8 bits from 128-bit LCG is detected as not random.
 *
 * But 64-bit version (8_8d) requires 6658042 points instead of 4096, i.e.
 * 1626 times more. And transition to 32-bit version gives about 20x
 * performance boost and allows to detect 128-bit LCGs even in fast batteries.
 *
 * 32-bit decimated birthday spacings test is much more sensitive than TMFn
 * from PractRand: it requires 1 GiB to detect 128-bit LCG with truncated 64 bits
 * (PractRand needs 128 GiB) and 32 GiB to detect 128-bit LCG with truncated 96 bits
 * (PractRand doesn't detect it at 32 TiB sample).
 *
 * @param obj   Analyzed generator and its state.
 * @param step  Decimation step: only 1 of `step` values will be used by the
 *              test. 4096 is enough to detect 128-bit LCG with truncation of
 *              lower 64 bits, 262144 is enough to detect the same PRNG with
 *              truncation of lower 96 bits.
 */
TestResults bspace4_8d_decimated_test(GeneratorState *obj, unsigned int step)
{
    TestResults ans = TestResults_create("bspace4_8d");
    const unsigned int nbits_total = 32;
    size_t len = bspace_calc_len(nbits_total);
    double lambda = bspace_calc_lambda(len, nbits_total);
    // Show information about the test
    obj->intf->printf("Birthday spacings test with decimation\n");
    obj->intf->printf("  ndims = 8; nbits_per_dim = 4; step = %u\n", step);
    obj->intf->printf("  nsamples = 1; len = %lld, lambda = %g\n",
        (unsigned long long) len, lambda);
    // Run the test
    uint32_t *u = calloc(len, sizeof(uint32_t));
    uint32_t *u_high_rev = calloc(len, sizeof(uint32_t));
    uint32_t *u_high_norev = calloc(len, sizeof(uint32_t));
    if (u == NULL || u_high_rev == NULL || u_high_norev == NULL) {
        fprintf(stderr, "***** bspace4_8d_decimated: not enough memory *****\n");
        free(u); free(u_high_rev); free(u_high_norev);
        exit(1);
    }

    for (size_t i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            uint64_t x = obj->gi->get_bits(obj->state);
            uint32_t x_hi4 = x >> (obj->gi->nbits - 4);
            // Take lower 4 bits
            u[i] <<= 4;
            u[i] |=  x & 0xF;
            // Take higher 4 bits with reversal
            u_high_rev[i] <<= 4;
            u_high_rev[i] |= reverse_bits4(x_hi4);
            // Take higher 4 bits without reversal
            u_high_norev[i] <<= 4;
            u_high_norev[i] |= x_hi4;
            // Decimation
            for (unsigned int k = 0; k < step - 1; k++) {
                (void) obj->gi->get_bits(obj->state);
            }
        }
    }
    ans.p = 1000.0;
    bspace4_8d_decimated_pvalue(&ans, "Lower bits:", u, len, lambda, obj->intf);
    bspace4_8d_decimated_pvalue(&ans, "Higher bits (reversed):",
        u_high_rev, len, lambda, obj->intf);
    bspace4_8d_decimated_pvalue(&ans, "Higher bits (no reverse):",
        u_high_norev, len, lambda, obj->intf);
    obj->intf->printf("  %-30s x = %6.0f; p = %g\n",
        "Final result:", ans.x, ans.p);
    obj->intf->printf("\n");
    free(u);
    free(u_high_rev);
    free(u_high_norev);
    return ans;
}

/////////////////////////////////////////////
///// CollisionOver test implementation /////
/////////////////////////////////////////////

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
 * @details Number of collision obeys the Poisson distribution with the next
 * mathematical expectance:
 *
 * \f[
 * \mu = d^t \left(\lambda - 1 + e^{-\lambda} \right)
 * \f]
 *
 * where \f$d\f$ is number of states per dimensions, \f$t\f$ is the number
 * of dimensions and \f$\lambda\f$ is so-called "density":
 *
 * \f[
 * \lambda = \frac{n - t + 1}{d^t}
 * \f]
 *
 * where \f$n\f$ is the number of points in the sample. The formula are correct
 * only when \f$ n \ll d^t \f$.
 *
 * The formula are taken from the TestU01 user manual.
 */
TestResults collisionover_test(GeneratorState *obj, const BSpaceNDOptions *opts)
{
    size_t n = 50000000;
    uint64_t *u = calloc(n, sizeof(uint64_t));
    if (u == NULL) {
        fprintf(stderr, "***** collisionover_test: not enough memory *****\n");
        exit(1);
    }
    uint64_t nstates_u64 = 1ull << opts->ndims * opts->nbits_per_dim;
    uint64_t Oi[4] = {0, 0, 0, 0};
    Oi[0] = nstates_u64;
    double nstates = nstates_u64;
    double lambda = (n - opts->ndims + 1.0) / nstates;
    double mu = nstates * (lambda - 1 + exp(-lambda));
    TestResults ans;
    ans.name = "CollisionOver";
    obj->intf->printf("CollisionOver test\n");
    obj->intf->printf("  ndims = %d; nbits_per_dim = %d; get_lower = %d\n",
        opts->ndims, opts->nbits_per_dim, opts->get_lower);
    obj->intf->printf("  nsamples = %lu; len = %lu, mu = %g * %d\n",
        opts->nsamples, n, mu, (int) opts->nsamples);

    ans.x = 0;
    for (unsigned long i = 0; i < opts->nsamples; i++) {
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
    double Ei = exp(-lambda) * nstates;
    obj->intf->printf("  Frequencies table (average per sample)\n");
    obj->intf->printf(" -----------------------------------------\n");
    obj->intf->printf("  %5s %16s %16s\n", "Freq", "Oi", "Ei");
    for (int i = 0; i < 4; i++) {
        uint64_t Oi_normed = (i == 0) ? Oi[i] : Oi[i] / opts->nsamples;
        obj->intf->printf("  %5d %16lld %16.1f\n", i, Oi_normed, Ei);
        Ei *= lambda / (i + 1.0);
    }
    obj->intf->printf(" -----------------------------------------\n");
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

static int gap_test_guard(GeneratorState *obj, const GapOptions *opts)
{
    int is_ok = 0;
    uint64_t beta = 1ull << (obj->gi->nbits - opts->shl);
    double p = 1.0 / (1 << opts->shl); // beta in the floating point format
    unsigned long long nsamples = -20.0 / log10(1.0 - p);
    for (unsigned long i = 0; i < nsamples; i++) {
        if (obj->gi->get_bits(obj->state) <= beta) {
            is_ok = 1;
            break;
        }
    }
    return is_ok;
}

/**
 * @brief Knuth's gap test for detecting lagged Fibonacci generators.
 * @details Gap is \f$ [0;\beta) \f$ where $\beta = 1 / (2^{shl}) \f$.
 */
TestResults gap_test(GeneratorState *obj, const GapOptions *opts)
{
    const double Ei_min = 10.0;
    double p = 1.0 / (1 << opts->shl); // beta in the floating point format
    uint64_t beta = 1ull << (obj->gi->nbits - opts->shl);
    uint64_t u;
    size_t ngaps = opts->ngaps;
    size_t nbins = log(Ei_min / (ngaps * p)) / log(1 - p);
    size_t *Oi = calloc(nbins + 1, sizeof(size_t));
    if (Oi == NULL) {
        fprintf(stderr, "***** gap_test: not enough memory *****\n");
        exit(1);
    }
    unsigned long long nvalues = 0;
    TestResults ans = TestResults_create("Gap");
    obj->intf->printf("Gap test\n");
    obj->intf->printf("  alpha = 0.0; beta = %g; shl = %u;\n", p, opts->shl);
    obj->intf->printf("  ngaps = %lu; nbins = %lu\n",
        (unsigned long) ngaps, (unsigned long) nbins);
    if (!gap_test_guard(obj, opts)) {
        obj->intf->printf("  Generator output doesn't hit the gap! p <= 1e-15\n");
        ans.p = 1.0e-15;
        ans.alpha = 1.0 - ans.p;
        ans.x = NAN;
        return ans;
    }
    for (size_t i = 0; i < ngaps; i++) {
        size_t gap_len = 0;
        u = obj->gi->get_bits(obj->state);
        nvalues++;
        while (u > beta) {
            gap_len++;
            u = obj->gi->get_bits(obj->state);
            nvalues++;
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
    obj->intf->printf("  Values processed: %llu (2^%.1f)\n",
        nvalues, sr_log2(nvalues));
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}

///////////////////////////////////////////////
///// Gap16 ('rda16') test implementation /////
///////////////////////////////////////////////



typedef struct {
    unsigned long long ngaps_total;
    unsigned long long ngaps_with0;
    unsigned long long ngaps_withrb;
    double p_total;
    double p_with0;
} GapFrequency;


/**
 * @brief Create array of gaps frequencies and theoretical probabilities.
 * @param nbins Number of bins (bin 0 for gap with length 0, bin 1 for gap
 * with length 1 etc.)
 * @param p Probability of number to be inside interval (e.g. for 16-bit word
 * to be equal to 0).
 */
static GapFrequency *GapFrequency_create_array(size_t nbins, double p)
{
    GapFrequency *gapfreq = calloc(nbins + 1, sizeof(GapFrequency));
    double p_gap = p;
    for (size_t i = 0; i < nbins; i++) {
        gapfreq[i].p_total = p_gap;
        p_gap *= 1.0 - p;
        if (i > 1) {
            gapfreq[i].p_with0 = 1.0 - binomial_pdf(0, i, p);
        } else if (i == 1) {
            gapfreq[i].p_with0 = p;
        } else if (i == 0) {
            gapfreq[i].p_with0 = 0.0;
        }
    }
    return gapfreq;
}

static double GapFrequency_calc_z_with0(const GapFrequency *gf)
{
    double alpha = binomial_cdf(gf->ngaps_with0, gf->ngaps_total, gf->p_with0);
    double pvalue = binomial_pvalue(gf->ngaps_with0, gf->ngaps_total, gf->p_with0);
    // Check if discretization error is big enough to take it into account
    // It can be e.g. rather sensitive in the case of binopdf(1085, 1085, 0.992085)
    // when mathematical expectance is 1076.4 and standard deviation is 2.91.
    // It means that 1085 will give p-value 1 but is still inside 4sigma
    if (pvalue < 1e-10 || alpha < 1e-10) {        
        double p_discrete = binomial_pdf(gf->ngaps_with0, gf->ngaps_total, gf->p_with0);
        if (p_discrete > 1e-10) {
            pvalue = p_discrete;
            alpha = 1 - pvalue;
        }
    }
    // Convert to normal distribution
    double z_w0 = (pvalue < 0.5) ? -stdnorm_inv(pvalue) : stdnorm_inv(alpha);
    if (z_w0 < -40.0) {
        z_w0 = -40.0;
    } else if (z_w0 > 40.0) {
        z_w0 = 40.0;
    }
    return z_w0;
}

static double gap16_sumsq_as_norm(const GapFrequency *gapfreq,
    size_t nbins, unsigned long long ngaps)
{
    double chi2emp = 0.0;
    for (size_t i = 0; i < nbins; i++) {
        double Oi = gapfreq[i].ngaps_total;
        double Ei = ngaps * gapfreq[i].p_total;
        chi2emp += pow(Oi - Ei, 2.0) / Ei;
    }
    return chi2_to_stdnorm_approx(chi2emp, nbins - 1);
}

void gap16_count0_mainloop(GapFrequency *gapfreq, GapFrequency *gapfreq_rb,
    size_t nbins, GeneratorState *obj, long long ngaps)
{
    int w16_per_num = obj->gi->nbits / 16;
    long long mod_mask = w16_per_num - 1, last0_pos = -1;
    uint64_t u = 0;
    long long *lastw16_pos = calloc(65536, sizeof(long long));
    for (size_t i = 0; i < 65536; i++) {
        lastw16_pos[i] = -1;
    }
    for (long long pos = 0; pos < ngaps; pos++) {
        // Generate 16-bit word
        if ((pos & mod_mask) == 0) {
            u = obj->gi->get_bits(obj->state);
        }
        uint64_t w16 = u & 0xFFFF;
        u >>= 16;
        // Update "last seen 0" position
        if (w16 == 0) {
            last0_pos = pos;
        }
        // Update gaps information (symmetric boundaries)
        if (lastw16_pos[w16] != -1) {
            // a) Get beginning and length
            long long gap_len = pos - lastw16_pos[w16] - 1;
            if (gap_len >= (long long) nbins) gap_len = nbins;    
            // b) Update frequency table
            if (last0_pos > lastw16_pos[w16]) {
                gapfreq[gap_len].ngaps_with0++;
            }
            gapfreq[gap_len].ngaps_total++;
        }
        // Update gaps information (asymmetric boundaries)
        if (last0_pos != -1) {
            // a) Get beginning and length
            long long gap_len = pos - last0_pos - 1;
            if (gap_len >= (long long) nbins) gap_len = nbins;
            // b0 Update frequency table
            if (lastw16_pos[w16] > last0_pos) {
                gapfreq_rb[gap_len].ngaps_with0++;
            }
            gapfreq_rb[gap_len].ngaps_total++;
        }
        // c) Update beginning
        lastw16_pos[w16] = pos;
    }
    free(lastw16_pos);
}


typedef struct {
    double z_max_tot;
    double z_max_w0;
    size_t ind_tot;
    size_t ind_w0;
} GapFrequencyStats;


static GapFrequencyStats gap16_count0_calc_zfreq(GapFrequency *gapfreq, size_t nbins, long long ngaps)
{
    GapFrequencyStats ans = {.z_max_w0 = 0.0, .z_max_tot = 0.0,
        .ind_tot = 0, .ind_w0 = 0};
    // Computation of p-values for all frequencies
    for (size_t i = 1; i < nbins; i++) {
        const GapFrequency *gf = &gapfreq[i];
        // Frequency of gap "with zeros" inside
        // Convert binomial distribution to normal through p and cdf/inv.cdf
        double z_w0 = GapFrequency_calc_z_with0(gf);
        if (fabs(z_w0) > fabs(ans.z_max_w0)) {
            ans.z_max_w0 = z_w0;
            ans.ind_w0 = i;
        }
        // Frequency of gaps: convert binomial distribution to normal
        // through central limiting theorem
        double mu = ngaps * gf->p_total;
        double s = sqrt(ngaps * gf->p_total * (1.0 - gf->p_total));
        double z_tot = (gf->ngaps_total - mu) / s;
        if (fabs(z_tot) > fabs(ans.z_max_tot)) {
            ans.z_max_tot = z_tot;
            ans.ind_tot = i;
        }
    }
    return ans;
}

/**
 * @brief Bonferroni correction for normally distributed variable
 * in gap16 test.
 */
static double gap16_z_bonferroni(double z, double p_gap)
{
    double z_corr;
    if (z < 0) {
        double p = stdnorm_cdf(z) / p_gap;
        z_corr = (p < 0.5) ? stdnorm_inv(p) : 0.0;
    } else {
        double p = stdnorm_cdf(-z) / p_gap;
        z_corr = (p < 0.5) ? -stdnorm_inv(p) : 0.0;
    }
    // Filter out possible infinities
    if (z_corr < -40.0) {
        return 40.0;
    } else if (z_corr > 40.0) {
        return 40.0;
    } else {
        return z_corr;
    }
}

/**
 * @brief Similar to rda16 ("repeat distance analysis") from gjrand, very sensitive
 * to additive and subtractive lagged Fibonacci generators.
 * @details Resembles classic gap test but analyses all possible (and equal length)
 * intervals simultaneously. It processes input as a sequence of 16-bit words and
 * uses 2^16 = 65536 intervals. The gap with length n looks like as:
 *
 *     [value; x_1; x_2; ...; x_n; value]
 *
 * Of course, gaps (subsequences) with different values at ends may be overlapping.
 * But just as in classic gap test their lengths are collected in one frequency
 * table. It can be considered as simultaneously running 65536 gap tests but
 * with shared frequency table for keeping their results.
 *
 * It also contains two powerful subtests:
 *
 * I. Zeros inside gaps. Calculates number of gaps with zeros  inside for each
 * gap length. It allows to detect additive and subtractive lagged Fibonacci
 * generators with huge lags.
 *
 * II. Gaps with asymmetric boundaries and right boundary duplicates inside.
 * These gaps has the next layout:
 *
 *     [0; x_1; x_2; ...; x_n; value] 
 *
 * Detects additive/subtractive lagged Fibonacci generators and SWB (subtract
 * with borrow) generators.
 */
TestResults gap16_count0_test(GeneratorState *obj, long long ngaps)
{
    TestResults ans = TestResults_create("gap16_count0");
    if (ngaps < 1000000) {
        return ans;
    }
    const double Ei_min = 1000.0, p = 1.0 / 65536.0;
    size_t nbins = log(Ei_min / (ngaps * p)) / log(1 - p);
    // Initialize frequency and position tables
    GapFrequency *gapfreq = GapFrequency_create_array(nbins, p);
    GapFrequency *gapfreq_rb = GapFrequency_create_array(nbins, p);
    // Print test info
    obj->intf->printf("gap16_count0 test\n");
    obj->intf->printf("  p = %g; ngaps = %llu; nbins = %lu\n",
        p, ngaps, (unsigned long) nbins);
    // Test main run
    gap16_count0_mainloop(gapfreq, gapfreq_rb, nbins, obj, ngaps);
    // Computation of p-value for sum of squares
    double z_sumsq_total = gap16_sumsq_as_norm(gapfreq, nbins, ngaps);
    obj->intf->printf("  z from chi2emp: %g; p = %g\n",
        z_sumsq_total, stdnorm_pvalue(z_sumsq_total));
    // Computation of p-values for all frequencties
    GapFrequencyStats gf = gap16_count0_calc_zfreq(gapfreq, nbins, ngaps);
    GapFrequencyStats gf_rb = gap16_count0_calc_zfreq(gapfreq_rb, nbins, ngaps);
    obj->intf->printf("  Frequency analysis for [value ... value] gaps with '0' inside:\n");
    obj->intf->printf("    z_max = %g; p = %g; alpha = %g; index = %d\n",
        gf.z_max_w0, stdnorm_pvalue(gf.z_max_w0), stdnorm_cdf(gf.z_max_w0), (int) gf.ind_w0);
    obj->intf->printf("    ngaps_total = %llu; ngaps_with0 = %llu\n",
        gapfreq[gf.ind_w0].ngaps_total, gapfreq[gf.ind_w0].ngaps_with0);
    obj->intf->printf("    p_total = %g; p_with0 = %g\n",
        gapfreq[gf.ind_w0].p_total, gapfreq[gf.ind_w0].p_with0);
    obj->intf->printf("  Frequency analysis for all [value ... value] gaps:\n");
    obj->intf->printf("    z_max = %g; p = %g; alpha = %g; index = %d\n",
        gf.z_max_tot, stdnorm_pvalue(gf.z_max_tot), stdnorm_cdf(gf.z_max_tot), (int) gf.ind_tot);
    obj->intf->printf("  Frequency analysis for [0 ... value] gaps with 'value' inside:\n");
    obj->intf->printf("    z_max = %g; p = %g; alpha = %g; index = %d\n",
        gf_rb.z_max_w0, stdnorm_pvalue(gf_rb.z_max_w0), stdnorm_cdf(gf_rb.z_max_w0), (int) gf_rb.ind_tot);
    obj->intf->printf("    ngaps_total = %llu; ngaps_with0 = %llu\n",
        gapfreq_rb[gf_rb.ind_w0].ngaps_total, gapfreq_rb[gf_rb.ind_w0].ngaps_with0);
    obj->intf->printf("    p_total = %g; p_with0 = %g\n",
        gapfreq_rb[gf_rb.ind_w0].p_total, gapfreq_rb[gf_rb.ind_w0].p_with0);
    obj->intf->printf("  Note: remember about Bonferroni correction!\n");
    obj->intf->printf("\n");
    // Make total p-value (with Bonferroni correction if required)
    double z_bonferroni = -stdnorm_inv(1e-4 * p);
    ans.x = z_sumsq_total;
    if (fabs(gf.z_max_w0) > z_bonferroni && fabs(gf.z_max_w0) > fabs(ans.x)) {
        ans.x = gap16_z_bonferroni(gf.z_max_w0, p);
    }
    if (fabs(gf.z_max_tot) > z_bonferroni && fabs(gf.z_max_tot) > fabs(ans.x)) {
        ans.x = gap16_z_bonferroni(gf.z_max_tot, p);
    }
    if (fabs(gf_rb.z_max_w0) > z_bonferroni && fabs(gf_rb.z_max_w0) > fabs(ans.x)) {
        ans.x = gap16_z_bonferroni(gf_rb.z_max_w0, p);
    }
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);
    // Output p-value
    obj->intf->printf("  x = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    // Free buffers
    free(gapfreq);
    free(gapfreq_rb);
    return ans;    
}

///////////////////////
///// Other tests /////
///////////////////////


static void sumcollector_calc_p(double *p, const int g, const int nmax)
{
    long double *g_mat = calloc((g + 1) * (nmax + 1), sizeof(long double));
    // g0(n) = d_{0n}
    g_mat[0] = 1.0;
    for (int i = 1; i <= nmax; i++) {
        g_mat[i] = 0.0;
    }
    // g_g(n) = 1 / factorial(g + 1)
    for (int i = 1; i <= g; i++) {
        g_mat[(nmax + 1) * i + i] = 1.0 / exp(sr_lgamma(i + 2));
    }
    // Recurrent formula
    for (int gi = 1; gi <= g; gi++) {
        for (int ni = gi + 1; ni <= nmax; ni++) {
            g_mat[(nmax + 1) * gi + ni] = (long double) (ni - gi + 1) / (ni + 1) * (
                g_mat[(nmax + 1) * (gi - 1) + (ni - 1)] +
                (long double) gi / (ni - gi) * g_mat[(nmax + 1) * gi + (ni - 1)]
            );
        }
    }
    for (int i = 0; i <= nmax; i++) {
        p[i] = g_mat[(nmax + 1) * g + i];
    }
    free(g_mat);
}

/**
 * @brief SumCollector test from TestU01 test suite.
 * @details Sensitive to the SWB (subtract with borrow) algorithm, in some cases
 * - even to its versions with lower "luxury levels".
 *
 * References:
 * 1. D. Ugrin-Sparac. On a distribution encountered in the renewal process based
 *    on uniform distribution // Glasnik Matematicki. 1990. V. 25. N 1. P.221-233.
 *    https://web.math.pmf.unizg.hr/glasnik/Vol/vol25no1.html
 * 2. Ugrin-Sparac G. Stochastic investigations of pseudo-random number
 *    generators // Computing. 1991. V. 46. P. 53-65.
 *    https://doi.org/10.1007/BF02239011
 */
TestResults sumcollector_test(GeneratorState *obj, unsigned long long nvalues)
{
    TestResults ans = TestResults_create("sumcollector");
    const unsigned int g = 10, nmax = 49;
    size_t y_cur = 0;
    uint64_t sum = 0, sum_max = (1ull << 32) * g;
    unsigned int shr = (obj->gi->nbits == 32) ? 0 : 32;
    unsigned long long *Oi_vec = calloc(nmax + 1, sizeof(unsigned long long));
    double *p_vec = calloc(nmax + 1, sizeof(double));    
    obj->intf->printf("SumCollector test\n");
    obj->intf->printf("  Number of values: %llu (2^%g)\n", nvalues, sr_log2(nvalues));
    sumcollector_calc_p(p_vec, g, nmax);

    for (unsigned long long i = 0; i < nvalues; i++) {
        uint64_t u = obj->gi->get_bits(obj->state) >> shr;
        uint64_t sum_new = u + sum;
        if (sum_new <= sum_max) {
            sum = sum_new;
            y_cur++;
        } else {
            if (y_cur > nmax) y_cur = nmax;
            Oi_vec[y_cur]++;
            sum = obj->gi->get_bits(obj->state) >> shr;
            y_cur = 1;
        }
    }
    unsigned long long Oi_sum = 0;
    for (int i = 0; i < 50; i++) {
        Oi_sum += Oi_vec[i];
    }

    unsigned long df = 0;
    ans.x = 0.0;
    for (size_t i = 0; i < 50; i++) {
        double Ei = p_vec[i] * Oi_sum;
        if (Ei >= 10.0) {
            double d = (Ei - Oi_vec[i]) * (Ei - Oi_vec[i]) / Ei;
            ans.x += d;
            df++;
        }
    }
    df--;
    ans.p = chi2_pvalue(ans.x, df);
    ans.alpha = chi2_cdf(ans.x, df);
    // Output p-value
    obj->intf->printf("  Number of sums: %llu (2^%g)\n", Oi_sum, sr_log2(Oi_sum));
    obj->intf->printf("  x = %g; p = %g; df = %lu\n", ans.x, ans.p, df);
    obj->intf->printf("\n");
    // Free buffers
    free(Oi_vec);
    free(p_vec);
    return ans;
}

/**
 * @brief A simplified version of `mod3` test from gjrand.
 * @details Detects some generators with nonlinear mappings, e.g. `flea32x1`
 * by Bob Jenkins or `ara32` from PractRand 0.94. These transformations are
 * based on combinations of additions and cyclic rotations; and multiplication
 * by 3 can be represented as a combination of left shift and addition.
 *
 * At larger samples (about 2^30 values) it catches 32-bit LCGs with (even
 * with prime modulus and shr3 (xorshift32).
 * @param nvalues Sample size, number of values (at least 2^27 is recommended).
 */
TestResults mod3_test(GeneratorState *obj, unsigned long long nvalues)
{
    TestResults ans = TestResults_create("mod3");
    const size_t ntuples = 19683; // 3^9
    unsigned long long *Oi = calloc(ntuples, sizeof(unsigned long long));
    uint32_t tuple = 0;
    obj->intf->printf("mod3 test\n");
    obj->intf->printf("  Sample size: %llu values\n", nvalues);
    for (int i = 0; i < 9; i++) {
        int d = obj->gi->get_bits(obj->state) % 3;
        tuple = (tuple * 3 + d) % ntuples;
    }
    for (unsigned long long i = 0; i < nvalues; i++) {
        Oi[tuple]++;
        int d = obj->gi->get_bits(obj->state) % 3;
        tuple = (tuple * 3 + d) % ntuples;
    }
    double Ei = (double) nvalues / (double) ntuples;
    ans.x = 0.0;
    for (size_t i = 0; i < ntuples; i++) {
        ans.x += (Oi[i] - Ei) * (Oi[i] - Ei) / Ei;
    }
    ans.x = chi2_to_stdnorm_approx(ans.x, ntuples - 1);
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);    
    obj->intf->printf("  z = %g; p = %g\n", ans.x, ans.p);
    obj->intf->printf("\n");
    return ans;
}

//////////////////////////////////////////
///// Frequency tests implementation /////
//////////////////////////////////////////

/**
 * @brief Monobit frequency test.
 */
TestResults monobit_freq_test(GeneratorState *obj)
{
    int sum_per_byte[256];
    unsigned long long len = 1ul << 28;
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
    for (unsigned long long i = 0; i < len; i++) {
        uint64_t u = obj->gi->get_bits(obj->state);
        for (unsigned int j = 0; j < nbytes; j++) {
            bitsum += sum_per_byte[u & 0xFF];
            u >>= 8;
        }
    }
    ans.x = fabs((double) bitsum) / sqrt(len * obj->gi->nbits);
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);
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
 * @brief Frequencies of n-bit words.
 */
TestResults nbit_words_freq_test(GeneratorState *obj,
    const NBitWordsFreqOptions *opts)
{
    size_t nbins = 1ull << opts->bits_per_word;
    unsigned int nwords_per_num = obj->gi->nbits / opts->bits_per_word;
    unsigned long long block_len = (1ull << (opts->bits_per_word)) * opts->average_freq / nwords_per_num;
    size_t nwords = nwords_per_num * block_len;
    uint64_t mask = nbins - 1;
    double *chi2 = calloc(opts->nblocks, sizeof(double));
    size_t *wfreq = calloc(nbins, sizeof(size_t));
    
    TestResults ans;
    if (opts->bits_per_word == 8) {
        ans.name = "BytesFreq";
        obj->intf->printf("Byte frequency test\n");
    } else {
        ans.name = "WordsFreq";
        obj->intf->printf("%u-bit words frequency test\n", opts->bits_per_word);
    }
    obj->intf->printf("  nblocks = %lld; block_len = %lld\n",
            (unsigned long long) opts->nblocks,
            (unsigned long long) block_len);
    for (size_t ii = 0; ii < opts->nblocks; ii++) {
        for (size_t i = 0; i < nbins; i++) {
            wfreq[i] = 0;
        }
        for (size_t i = 0; i < block_len; i++) {
            uint64_t u = obj->gi->get_bits(obj->state);        
            for (size_t j = 0; j < nwords_per_num; j++) {
                wfreq[u & mask]++;
                u >>= opts->bits_per_word;
            }
        }
        chi2[ii] = 0;
        double Ei = (double) nwords / (double) nbins;
        for (size_t i = 0; i < nbins; i++) {
            chi2[ii] += pow(wfreq[i] - Ei, 2.0) / Ei;
        }
    }
    // Kolmogorov-Smirnov test
    qsort(chi2, opts->nblocks, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < opts->nblocks; i++) {
        double f = chi2_cdf(chi2[i], nbins - 1);
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / opts->nblocks - f;
        double Dminus = f - idbl / opts->nblocks;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double K = sqrt(opts->nblocks) * D + 1.0 / (6.0 * sqrt(opts->nblocks));
    ans.x = K;
    ans.p = ks_pvalue(K);
    ans.alpha = 1.0 - ans.p;
    obj->intf->printf("  K = %g, p = %g\n", K, ans.p);
    obj->intf->printf("\n");
    free(chi2);
    return ans;
}



/**
 * @brief Bytes frequency test.
 */
TestResults byte_freq_test(GeneratorState *obj)
{
    NBitWordsFreqOptions opts = {.bits_per_word = 8, .average_freq = 1024,
        .nblocks = 4096};
    return nbit_words_freq_test(obj, &opts);
}

/**
 * @brief 16-bit words frequency test.
 */
TestResults word16_freq_test(GeneratorState *obj)
{
    NBitWordsFreqOptions opts = {.bits_per_word = 16, .average_freq = 32,
        .nblocks = 4096};
    return nbit_words_freq_test(obj, &opts);
}
