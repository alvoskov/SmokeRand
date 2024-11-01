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
    if (u == NULL) {
        fprintf(stderr, "***** bspace32_nd_test: not enough memory *****\n");
        exit(1);
    }
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
    if (u == NULL) {
        fprintf(stderr, "***** bspace64_nd_test: not enough memory *****\n");
        exit(1);
    }
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
    if (u == NULL) {
        fprintf(stderr, "***** collisionover_test: not enough memory *****\n");
        exit(1);
    }
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
TestResults gap_test(GeneratorState *obj, const GapOptions *opts)
{
    const double Ei_min = 10.0;
    double p = 1.0 / (1 << opts->shl); // beta in the floating point format
    uint64_t beta = 1ull << (obj->gi->nbits - opts->shl);
    uint64_t u;
    size_t ngaps = opts->ngaps;
    size_t nbins = log(Ei_min / (ngaps * p)) / log(1 - p);
    size_t *Oi = calloc(nbins + 1, sizeof(size_t));
    unsigned long long nvalues = 0;
    TestResults ans;
    ans.name = "Gap";
    obj->intf->printf("Gap test\n");
    obj->intf->printf("  alpha = 0.0; beta = %g; shl = %u;\n", p, opts->shl);
    obj->intf->printf("  ngaps = %llu; nbins = %llu\n", ngaps, nbins);
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
    obj->intf->printf("  Values processed: %llu (2^%.1f)\n", nvalues, log2(nvalues));
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
 * @brief Frequencies of n-bit words.
 */
TestResults nbit_words_freq_test(GeneratorState *obj,
    const unsigned int bits_per_word, const unsigned int average_freq)
{
    size_t block_len = (1ull << (bits_per_word)) * average_freq;
    size_t nblocks = 4096;
    unsigned int nwords_per_num = obj->gi->nbits / bits_per_word;
    unsigned int nwords = nwords_per_num * block_len;
    size_t nbins = 1ull << bits_per_word;
    uint64_t mask = nbins - 1;
    double *chi2 = calloc(nblocks, sizeof(double));
    size_t *wfreq = calloc(nwords, sizeof(size_t));
    
    TestResults ans;
    if (bits_per_word == 8) {
        ans.name = "BytesFreq";
        obj->intf->printf("Byte frequency test\n");
    } else {
        ans.name = "WordsFreq";
        obj->intf->printf("%u-bit words frequency test\n", bits_per_word);
    }
    obj->intf->printf("  nblocks = %lld; block_len = %lld\n",
            (unsigned long long) nblocks,
            (unsigned long long) block_len);
    for (size_t ii = 0; ii < nblocks; ii++) {
        for (size_t i = 0; i < nbins; i++) {
            wfreq[i] = 0;
        }
        for (size_t i = 0; i < block_len; i++) {
            uint64_t u = obj->gi->get_bits(obj->state);        
            for (size_t j = 0; j < nwords_per_num; j++) {
                wfreq[u & mask]++;
                u >>= 8;
            }
        }
        chi2[ii] = 0;
        double Ei = (double) nwords / (double) nbins;
        for (size_t i = 0; i < nbins; i++) {
            chi2[ii] += pow(wfreq[i] - Ei, 2.0) / Ei;
        }
    }
    // Kolmogorov-Smirnov test
    qsort(chi2, nblocks, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < nblocks; i++) {
        double f = chi2_cdf(chi2[i], nbins - 1);
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



/**
 * @brief Bytes frequency test.
 */
TestResults byte_freq_test(GeneratorState *obj)
{
    return nbit_words_freq_test(obj, 8, 256);
}

/**
 * @brief 16-bit words frequency test.
 */
TestResults word16_freq_test(GeneratorState *obj)
{
    return nbit_words_freq_test(obj, 16, 16);
}

////////////////////////////////////////////////////
///// Hamming weight (DC6) test implementation /////
////////////////////////////////////////////////////


typedef struct _ByteStreamGenerator {
    const GeneratorInfo *gen; ///< Used generator
    void *state; ///< PRNG state
    uint64_t buffer; ///< Buffer with PRNG raw data
    size_t nbytes; ///< Number of bytes returned by the generator
    size_t bytes_left; ///< Number of bytes left in the buffer
    uint8_t (*get_byte)(struct _ByteStreamGenerator *obj);
} ByteStreamGenerator;


uint8_t ByteStreamGenerator_getbyte(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = obj->gen->get_bits(obj->state);
        obj->bytes_left = obj->nbytes;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}

uint8_t ByteStreamGenerator_getbyte_low1(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = 0;
        for (int i = 0; i < 64; i++) {
            obj->buffer = (obj->buffer >> 1) | (obj->gen->get_bits(obj->state) << 63);
        }
        obj->bytes_left = 8;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}

uint8_t ByteStreamGenerator_getbyte_low8(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = 0;
        for (int i = 0; i < 8; i++) {
            obj->buffer = (obj->buffer >> 8) | (obj->gen->get_bits(obj->state) << 56);
        }
        obj->bytes_left = 8;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}



/**
 * @details It DOESN'T OWN the PRNG state kept in gs->state!
 */
void ByteStreamGenerator_init(ByteStreamGenerator *obj,
    GeneratorState *gs, UseBitsMode use_bits)
{
    obj->gen = gs->gi;
    obj->state = gs->state;
    obj->nbytes = gs->gi->nbits / 8;
    obj->bytes_left = 0;
    switch (use_bits) {
    case use_bits_all:
        obj->get_byte = ByteStreamGenerator_getbyte;
        break;

    case use_bits_low8:
        obj->get_byte = ByteStreamGenerator_getbyte_low8;
        break;

    case use_bits_low1:
        obj->get_byte = ByteStreamGenerator_getbyte_low1;
        break;

    default:
        obj->get_byte = ByteStreamGenerator_getbyte;
        break;
    }
}




/**
 * @brief Keeps the Hamming weight tuple counter and theoretical probability
 */
typedef struct {
    unsigned long long count; ///< Empirical frequency (counter)
    double p; ///< Probability of the corresponding tuple
} HammingWeightsTuple;


typedef struct {
    HammingWeightsTuple *tuples;
    size_t len;
} HammingTuplesTable;


void HammingTuplesTable_init(HammingTuplesTable *obj,
    unsigned int code_nbits, unsigned int tuple_size,
    const double *code_to_prob)
{
    const uint64_t code_mask = (1ull << code_nbits) - 1;
    obj->len = 1ull << code_nbits * tuple_size;
    obj->tuples = calloc(obj->len, sizeof(HammingWeightsTuple));
    // Calculate probabilities of tuples
    // (idea is taken from PractRand)
    for (size_t i = 0; i < obj->len; i++) {
        uint64_t tmp = i;
        obj->tuples[i].p = 1.0;
        for (unsigned int j = 0; j < tuple_size; j++) {
            obj->tuples[i].p *= code_to_prob[tmp & code_mask];
            tmp >>= code_nbits;
        }
    }
}

size_t HammingWeightsTuple_reduce_table(HammingWeightsTuple *info, double Ei_min, size_t len)
{
    // Calculate minimal probability for the bin
    unsigned long long count_total = 0;
    for (size_t i = 0; i < len; i++) {
        count_total += info[i].count;
    }
    double p_min = Ei_min / (double) count_total;
    // Bins reduction
    int reduced = 0;
    while (!reduced) {
        size_t out_len = 0, inp_pos = 0;
        reduced = 1;
        while (inp_pos < len) {
            if (info[inp_pos].p >= p_min) {
                // Good bin
                info[out_len++] = info[inp_pos++];
            } else if (inp_pos != len - 1 &&
                (out_len == 0 || info[out_len - 1].p > info[inp_pos + 1].p)) {
                // Bin with low p: attach to the neighbour from the right
                // E.g.: 26 27 | 5* 2 50 => 27 27 7 | ? 50*
                info[out_len] = info[inp_pos++];
                info[out_len].p += info[inp_pos].p;
                info[out_len].count += info[inp_pos].count;
                out_len++; inp_pos++;
                reduced = 0;
            } else {
                // Bin with low p: attach to the neighbour from the left
                // E.g.: 26 27 | 5* 130 50 => 26 32 | ? 130* 50
                info[out_len - 1].p += info[inp_pos].p;
                info[out_len - 1].count += info[inp_pos].count;
                inp_pos++;
                reduced = 0;
            }
        }
        len = out_len;
    }
    return len;
}

TestResults HammingTuplesTable_get_results(HammingTuplesTable *obj)
{
    // Concatenate low-populated bins
    obj->len = HammingWeightsTuple_reduce_table(obj->tuples, 50.0, obj->len);
    // chi2 test (with more accurate g-test statistics)
    // chi2emp = 2\sum_i O_i * ln(O_i / E_i)
    unsigned long long count_total = 0;
    for (size_t i = 0; i < obj->len; i++) {
        count_total += obj->tuples[i].count;
    }
    TestResults ans = {.x = 0.0, .p = NAN, .name = "hamming_dc6"};
    for (size_t i = 0; i < obj->len; i++) {
        double Ei = count_total * obj->tuples[i].p;
        double Oi = obj->tuples[i].count;
        if (Oi > DBL_EPSILON) {
            ans.x += 2.0 * Oi * log(Oi / Ei);
        }
    }
    ans.x = chi2_to_stdnorm_approx(ans.x, obj->len - 1);
    ans.p = stdnorm_pvalue(ans.x);
    ans.alpha = stdnorm_cdf(ans.x);
    return ans;
}

void HammingTuplesTable_free(HammingTuplesTable *obj)
{
    free(obj->tuples);
    obj->tuples = NULL;
}


/**
 * @brief Creates the table of Hamming weights for bytes.
 */
uint8_t *create_bytes_hamming_weights()
{
    uint8_t *hw = calloc(256, sizeof(uint8_t));
    for (int i = 0; i < 256; i++) {        
        for (int j = 0; j < 8; j++) {
            if (i & (1 << j)) {
                hw[i]++;
            }
        }
    }
    return hw;
}


unsigned long long hamming_dc6_nsamples_to_ntuples(unsigned long long nsamples,
    unsigned int nbits, UseBitsMode use_bits)
{
    unsigned long long ntuples = nsamples;
    switch (use_bits) {
    case use_bits_all:
        ntuples = nsamples;
        break;
    case use_bits_low8:
        ntuples = nsamples * 8 / nbits;
        break;
    case use_bits_low1:
        ntuples = nsamples / nbits;
        break;
    default:
        ntuples = nsamples;
        break;
    }
    return ntuples;
}

/**
 * @brief Hamming weights based test (modification of DC6-9x1Bytes-1
 * from PractRand by Chris Doty-Humphrey).
 * @details The test was suggested by Chris Doty-Humphrey, the developer
 * of PractRand test suite. Actually it is a family of algorithms that
 * can analyse bytes, 16-bit, 32-bit and 64-bit chunks. The DC6-9x1Bytes-1
 * modification works with bytes and includes the next steps:
 *
 * 1. Input stream is analysed as a sequence of bytes.
 * 2. Each byte is converted to its Hamming weight (number of 1's).
 * 3. Hamming weight is transformed two-bit binary code using a pre-defined
 *    table. The default settings are tuples from 9 codes.
 *    (number of tuple types: `ntuples = 4^9 = 262144`)
 * 4. A set of overlapping tuples (nsamples, much greater than ntuples) is
 *    formed from the stream, the principle is similar to G.Marsaglia
 *    "monkey tests". Number of tuples of each type is counted, histogram,
 *    i.e. Ei values, are formed. The histogram has `ntuples` bins.
 * 5. The obtained histogram is analysed for bins with small amounts of
 *    elements (<= Ei_min = 25). Such bins are concatenated with neighbours.
 * 6. g-test (slightly more accurate modification of chi-square test) is
 *    appled to the obtained histogram. Then the obtained chi2emp is
 *    transformed to the zemp (standard normal distribution) by some
 *    asymptotic formula.
 * 7. The obtained zemp is recalibrated by some empirical data to improve
 *    the p-values accuracy. The inaccuracy is caused by possible dependencies
 *    between different frequencies (just as in "monkey test"). But the step
 *    with bins concatenation makes theoretical approach problematic. So
 *    Monte-Carlo approach with CSPRNG (HC-256?) was used.
 * 8. p-value is calculated for the obtained zemp.
 *
 * WARNING! This description is reverse engineered from the PractrRand source
 * code by A.L.Voskov and may be inaccurate. Some simplifications were made:
 *
 * 1. Codes reordering was excluded.
 * 2. All generalizations for different shifts/settings were excluded (they
 *    are not used in the default PractRand tests battery).
 * 3. Sophisticated scheme with bits mixing during tuples formation was
 *    excluded (probably the author had some ideas about sorting by
 *    probabilities?)
 * 4. A simpler and less accurate recalibration scheme was developed and used.
 *
 * The test easily detects low-grade PRNGs such as lcg69069, randu, 32-bit
 * and 64-bit xorshift. If only lower bit is analysed - it detects
 * SWB and SWBW (Subtract-with-Borrow + "Weyl sequence"). SWB passes BigCrush
 * but not PractRand. For such detection the ordering of lower bits in
 * the output is very important.
 * 
 * References:
 *
 * 1. Chris Doty-Humphrey. PractRand https://pracrand.sourceforge.net/
 *
 */
TestResults hamming_dc6_test(GeneratorState *obj, const HammingDc6Options *opts)
{
    // 2-bit codes for Hamming weights (taken from PractRand)
    //                                    0  1  2  3  4  5  6  7  8
    static const uint32_t hw_to_code[] = {0, 0, 1, 1, 2, 2, 3, 3, 0};
    // binopdf(0:8,8,0.5) * 256:          1  8 28 56 70 56 28  8  1
    static const double code_to_prob[] = {10.0/256.0, 84.0/256.0, 126.0/256.0, 36.0/256.0};
    // Parameters for 18-bit tuple with 2-bit digits
    static const unsigned int code_nbits = 2, tuple_size = 9;
    static const uint64_t tuple_mask = (1ull << code_nbits * tuple_size) - 1;
    //
    HammingTuplesTable table;
    HammingTuplesTable_init(&table, code_nbits, tuple_size, code_to_prob);
    uint8_t *hw = create_bytes_hamming_weights();
    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    uint8_t cur_weight; // Current Hamming weight
    unsigned long long ntuples = hamming_dc6_nsamples_to_ntuples(opts->nsamples,
        obj->gi->nbits, opts->use_bits);
    ByteStreamGenerator bs;
    ByteStreamGenerator_init(&bs, obj, opts->use_bits);
    obj->intf->printf("Hamming weights based tests (DC6)\n");
    obj->intf->printf("  Sample size, bytes: %llu\n", opts->nsamples);
    switch (opts->use_bits) {
    case use_bits_all:
        obj->intf->printf("  Used bits: all\n");
        break;
    case use_bits_low1:
        obj->intf->printf("  Used bits: bit 0\n");
        break;
    case use_bits_low8:
        obj->intf->printf("  Used bits: bits 7..0\n");
        break;
    }    
    // Pre-fill tuple
    cur_weight = hw[ByteStreamGenerator_getbyte(&bs)];
    for (unsigned int i = 0; i < tuple_size; i++) {
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = hw[bs.get_byte(&bs)];
    }
    // Generate other overlapping tuples
    for (unsigned long long i = 0; i < ntuples; i++) {
        // Process current tuple
        table.tuples[tuple].count++;
        // Update tuple
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = hw[bs.get_byte(&bs)];
    }
    // Convert tuples counters to the test results (p-value etc.)
    TestResults res = HammingTuplesTable_get_results(&table);
    obj->intf->printf("  zemp = %g, p = %g\n", res.x, res.p);
    obj->intf->printf("\n");
    HammingTuplesTable_free(&table);
    free(hw);
    return res;
}
