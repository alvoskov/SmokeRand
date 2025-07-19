/**
 * @file hwtests.c
 * @brief Hamming weights bases tests implementation, mainly DC6.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/hwtests.h"
#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

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
    case USE_BITS_ALL:
        obj->get_byte = ByteStreamGenerator_getbyte;
        break;

    case USE_BITS_LOW8:
        obj->get_byte = ByteStreamGenerator_getbyte_low8;
        break;

    case USE_BITS_LOW1:
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
    if (obj->tuples == NULL) {
        fprintf(stderr, "***** HammingTuplesTable_init: not enough memory *****\n");
        exit(1);
    }
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

/**
 * @brief Calculates statistics for Hamming DC6 test using the filled table
 * of overlapping tuples frequencies. The g-test (refined chi-square test)
 * is used. Obtained random value is converted to the normal distribution
 * using asymptotic approximation from the `chi2_to_stdnorm_approx` function.
 * @details The original DC6 test from PractRand also used recalibration
 * procedure: correction of obtained \f$z_\mathrm{emp}\f$ using some
 * corrections based on percentile tables obtained by Monte-Carlo method using
 * CSPRNG. Experiments showed that increasing \f$E_{i,\mathrm{min}}\f$ from
 * 25 to 100 makes the output distribution much closer to standard normal.
 * It is still slightly biased: \f$\mu\approx-0.15\f$, \f$\sigma\approx 1.0\f$.
 * Q-Q plot shows that it is normal. And this bias was not taken into account.
 */
TestResults HammingTuplesTable_get_results(HammingTuplesTable *obj)
{
    // Concatenate low-populated bins
    const size_t nbins_max = 250000;
    double Ei_min = 250.0;
    do {
        obj->len = HammingWeightsTuple_reduce_table(obj->tuples, Ei_min, obj->len);
        Ei_min *= 2;
    } while (obj->len > nbins_max);
    // chi2 test (with more accurate g-test statistics)
    // chi2emp = 2\sum_i O_i * ln(O_i / E_i)
    unsigned long long count_total = 0;
    for (size_t i = 0; i < obj->len; i++) {
        count_total += obj->tuples[i].count;
    }
    TestResults ans = TestResults_create("hamming_ot");
    ans.x = 0;
    for (size_t i = 0; i < obj->len; i++) {
        double Ei = count_total * obj->tuples[i].p;
        double Oi = (double) obj->tuples[i].count;
        if (Oi > DBL_EPSILON) {
            ans.x += 2.0 * Oi * log(Oi / Ei);
        }
    }
    ans.penalty = PENALTY_HAMMING_OT;
    ans.x = sr_chi2_to_stdnorm_approx(ans.x, (unsigned long) (obj->len - 1));
    ans.p = sr_stdnorm_pvalue(ans.x);
    ans.alpha = sr_stdnorm_cdf(ans.x);
    return ans;
}

void HammingTuplesTable_destruct(HammingTuplesTable *obj)
{
    free(obj->tuples);
    obj->tuples = NULL;
}


/**
 * @brief Converts number of samples (bytes or words) to number of generated
 * tuples (not tuples types)
 */
unsigned long long hamming_ot_nbytes_to_ntuples(unsigned long long nbytes,
    unsigned int nbits, HammingOtMode mode)
{
    unsigned long long ntuples = nbytes;
    // Note: tuples are overlapping, i.e. one new tuple digit means
    // one new tuple.
    switch (mode) {
    case HAMMING_OT_VALUES:
        // Each PRNG output is converted to tuple digits
        ntuples = nbytes * 8 / nbits;
        break;
    case HAMMING_OT_BYTES:
        // Each byte is converted to tuple digit
        ntuples = nbytes;
        break;
    case HAMMING_OT_BYTES_LOW8:
        // Only lower bytes of PRNG outputs are converted to tuple digits
        ntuples = nbytes * 8 / nbits;
        break;
    case HAMMING_OT_BYTES_LOW1:
        // Only lower bits of PRNG outputs are converted to tuple digits
        ntuples = nbytes / nbits;
        break;
    default:
        ntuples = nbytes;
        break;
    }
    return ntuples;
}

/**
 * @brief Print the information about "Hamming DC6" test settings, especially
 * about input stream processing mode.
 */
static void hamming_ot_test_print_info(GeneratorState *obj, const HammingOtOptions *opts,
    unsigned long long ntuples)
{
    obj->intf->printf("Hamming weights based tests (overlapping tuples)\n");
    obj->intf->printf("  Sample size, bytes:     %llu (2^%.2f or 10^%.2f)\n",
        opts->nbytes, sr_log2((double) opts->nbytes), log10((double) opts->nbytes));
    obj->intf->printf("  Tuples to be generated: %llu (2^%.2f or 10^%.2f)\n",
        ntuples, sr_log2((double) ntuples), log10((double) ntuples));
    switch (opts->mode) {
    case HAMMING_OT_VALUES:
        obj->intf->printf("  Mode: process %d-bit words of PRNG output directly\n",
            (int) obj->gi->nbits);
        break;
    case HAMMING_OT_BYTES:
        obj->intf->printf("  Mode: process PRNG output as a stream of bytes\n",
            (int) obj->gi->nbits);
        break;
    case HAMMING_OT_BYTES_LOW1:
        obj->intf->printf("  Mode: byte stream made of lower bits (bit 0) of PRNG values\n");
        break;
    case HAMMING_OT_BYTES_LOW8:
        obj->intf->printf("  Mode: byte stream made of 8 lower bits (bit 7..0) of PRNG values\n");
        break;
    }
}

/**
 * @brief Fills tables for conversion of Hamming weights to its codes (tuple digits)
 * Takes into account both output size of PRNG (32 or 64 bits) and output stream mode
 * (bytes or 32/64-bit words).
 * @param[in]  obj          Information about used PRNG and its state
 * @param[in]  opts         Hamming overlapping tuples test options.
 * @param[out] code_to_prob Buffer for probabilities of codes. They are calculated
 *                          from the binomial distribution. It is supposed that
 *                          only 4 codes (0, 1, 2, 3) are possible
 * @return Pointer to the table of codes of Hamming weights.
 */
static const uint8_t *hamming_ot_fill_hw_tables(GeneratorState *obj,
    const HammingOtOptions *opts, double *code_to_prob)
{
    // Select the right table for recoding Hamming weights to codes
    int nweights = 0;
    const uint8_t *hw = NULL;
    if (opts->mode != HAMMING_OT_VALUES) {
        // 2-bit codes for Hamming weights (taken from PractRand)
        //                                   0  1  2  3  4  5  6  7  8
        static const uint8_t hw_to_code[] = {0, 0, 1, 1, 2, 2, 3, 3, 0};
        hw = hw_to_code;
        nweights = 9;
        // binopdf(0:8,8,0.5) * 256:          1  8 28 56 70 56 28  8  1
    } else if (obj->gi->nbits == 32) {
        // Our custom table of hw->code table for 32-bit words
        // sum(binopdf(0:13, 32,0.5)) ans = 0.1885
        // sum(binopdf(14:15,32,0.5)) ans = 0.2415
        // sum(binopdf(16:17,32,0.5)) ans = 0.2717
        // sum(binopdf(18:32,32,0.5)) ans = 0.2983
        static const uint8_t hw32_to_code[] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0-7
            0, 0, 0, 0, 0, 0, 1, 1, // 8-15
            2, 2, 3, 3, 3, 3, 3, 3, // 16-23
            3, 3, 3, 3, 3, 3, 3, 3,  3}; // 24-32
        hw = hw32_to_code;
        nweights = 33;
    } else if (obj->gi->nbits == 64) {
        // Our custom table of hw-code table for 64-bit words
        // sum(binopdf(0:28,64,0.5)) ans = 0.1909
        // sum(binopdf(29:31,64,0.5)) ans = 0.2595
        // sum(binopdf(32:35,64,0.5)) ans = 0.3588
        // sum(binopdf(36:64,64,0.5)) ans = 0.1909
        static const uint8_t hw64_to_code[] = {
            0, 0, 0, 0, 0, 0, 0, 0, // 0-7
            0, 0, 0, 0, 0, 0, 0, 0, // 8-15
            0, 0, 0, 0, 0, 0, 0, 0, // 16-23
            0, 0, 0, 0, 0, 1, 1, 1, // 24-31
            2, 2, 2, 2, 3, 3, 3, 3, // 32-39
            3, 3, 3, 3, 3, 3, 3, 3, // 40-47
            2, 2, 3, 3, 3, 3, 3, 3, // 48-57
            3, 3, 3, 3, 3, 3, 3, 3,  3}; // 56-64
        hw = hw64_to_code;
        nweights = 65;
    } else {
        fprintf(stderr, "Internal error");
        exit(1);
    }
    // Calculate probabilities for codes using p.d.f.
    // for binomial distribution
    for (int i = 0; i < 4; i++) {
        code_to_prob[i] = 0.0;
    }
    for (int i = 0; i < nweights; i++) {
        code_to_prob[hw[i]] += sr_binomial_pdf(i, nweights - 1, 0.5);
    }
    return hw;
}



/**
 * @brief Hamming weights based test that uses overlapping tuple; it is
 * a modification of DC6-9x1Bytes-1 from PractRand by Chris Doty-Humphrey.
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
 * Also a new mode that processes the whole output of PRNG not byte-by-byte
 * was implemented. So the name was changed from "DC6" to "ot" (overlapping
 * tuples): it is not a reimplementation of DC6 from PractRand but similar
 * test based on ideas from it.
 *
 * The test easily detects low-grade PRNGs such as lcg69069, randu, 32-bit,
 * 64-bit and 128-bit xorshift without output scramblers. If only lower bit is
 * analysed - it detects SWB and SWBW (Subtract-with-Borrow + "Weyl sequence").
 * SWBW passes BigCrush but not PractRand. For such detection the ordering of
 * lower bits in the output is very important.
 * 
 * References:
 *
 * 1. Chris Doty-Humphrey. PractRand https://pracrand.sourceforge.net/
 * 2. Blackman D., Vigna S. A New Test for Hamming-Weight Dependencies //
 *    ACM Trans. Model. Comput. Simul. 2022. V. 32, N. 3, Article 19
 *    https://doi.org/10.1145/3527582
 */
TestResults hamming_ot_test(GeneratorState *obj, const HammingOtOptions *opts)
{
    // Our custom table of hw->code table for 32-bit words
    double code_to_prob[4];
    const uint8_t *hw_to_code = hamming_ot_fill_hw_tables(obj, opts, code_to_prob);
    // Parameters for 18-bit tuple with 2-bit digits
    static const unsigned int code_nbits = 2, tuple_size = 9;
    const uint64_t tuple_mask = (1ull << code_nbits * tuple_size) - 1;
    //
    HammingTuplesTable table;
    HammingTuplesTable_init(&table, code_nbits, tuple_size, code_to_prob);

    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    uint8_t cur_weight; // Current Hamming weight
    unsigned long long ntuples = hamming_ot_nbytes_to_ntuples(opts->nbytes,
        obj->gi->nbits, opts->mode);
    hamming_ot_test_print_info(obj, opts, ntuples);
    obj->intf->printf("  Used probabilities for codes:\n");
    for (int i = 0; i < 4; i++) {
        obj->intf->printf("    p(%d) = %10.8f\n", i, code_to_prob[i]);
    }
    if (opts->mode == HAMMING_OT_VALUES) {
        // Process input as a sequence of 32/64-bit words
        // Pre-fill tuple
        cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        for (unsigned int i = 0; i < tuple_size; i++) {
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        }
        // Generate other overlapping tuples
        for (unsigned long long i = 0; i < ntuples; i++) {
            // Process current tuple
            table.tuples[tuple].count++;
            // Update tuple
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
        }
    } else {
        ByteStreamGenerator bs;
        if (opts->mode == HAMMING_OT_BYTES) {
            ByteStreamGenerator_init(&bs, obj, USE_BITS_ALL);
        } else if (opts->mode == HAMMING_OT_BYTES_LOW8) {
            ByteStreamGenerator_init(&bs, obj, USE_BITS_LOW8);
        } else if (opts->mode == HAMMING_OT_BYTES_LOW1) {
            ByteStreamGenerator_init(&bs, obj, USE_BITS_LOW1);
        } else {
            fprintf(stderr, "Internal error");
            exit(1);
        }
        // Pre-fill tuple
        cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        for (unsigned int i = 0; i < tuple_size; i++) {
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        }
        // Generate other overlapping tuples
        for (unsigned long long i = 0; i < ntuples; i++) {
            // Process current tuple
            table.tuples[tuple].count++;
            // Update tuple
            tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
            cur_weight = get_byte_hamming_weight(bs.get_byte(&bs));
        }
    }
    // Convert tuples counters to the test results (p-value etc.)
    obj->intf->printf("   Number of tuples types: %llu\n",
        (unsigned long long) table.len);
    TestResults res = HammingTuplesTable_get_results(&table);
    obj->intf->printf("   Number of bins after reduction: %llu\n",
        (unsigned long long) table.len);
    obj->intf->printf("  zemp = %g, p = %g\n", res.x, res.p);
    obj->intf->printf("\n");
    HammingTuplesTable_destruct(&table);
    return res;
}


/**
 * @brief Fill tables for conversion of Hamming weights to codes; and
 * codes - to probabilities.
 */
static void hamming_ot_long_fill_hw_tables(unsigned short *hw_to_code,
    double *code_to_prob, int bits_per_word)
{
    const int nweights = bits_per_word + 1;
    if (bits_per_word == 128) {
        // 0:59 | 60:64 | 65:68 | 69:128 
        for (int i = 0;  i <= 59; i++)  hw_to_code[i] = 0;
        for (int i = 60; i <= 64; i++)  hw_to_code[i] = 1;
        for (int i = 65; i <= 68; i++)  hw_to_code[i] = 2;
        for (int i = 69; i <= 128; i++) hw_to_code[i] = 3;
    } else if (bits_per_word == 256) {
        // 0:122 | 123:127 | 128:133 | 134:256
        for (int i = 0;   i <= 122; i++) hw_to_code[i] = 0;
        for (int i = 123; i <= 127; i++) hw_to_code[i] = 1;
        for (int i = 128; i <= 133; i++) hw_to_code[i] = 2;
        for (int i = 134; i <= 256; i++) hw_to_code[i] = 3;
    } else if (bits_per_word == 512) {
        // 0:247 | 248:255 | 256:264 | 265:512 
        for (int i = 0;   i <= 247; i++) hw_to_code[i] = 0;
        for (int i = 248; i <= 255; i++) hw_to_code[i] = 1;
        for (int i = 256; i <= 264; i++) hw_to_code[i] = 2;
        for (int i = 265; i <= 512; i++) hw_to_code[i] = 3;
    } else if (bits_per_word == 1024) {
        // 0:500 | 501:511 | 512:523 | 524:1024 
        for (int i = 0;   i <= 500; i++)  hw_to_code[i] = 0;
        for (int i = 501; i <= 511; i++)  hw_to_code[i] = 1;
        for (int i = 512; i <= 523; i++)  hw_to_code[i] = 2;
        for (int i = 524; i <= 1024; i++) hw_to_code[i] = 3;
    } else {
        // Unknown word size: do something that will cause failure
        for (int i = 0; i < nweights; i++) {
            hw_to_code[0] = 0;
        }
    }
    // Calculate probabilities for codes using p.d.f.
    // for binomial distribution
    for (int i = 0; i < 4; i++) {
        code_to_prob[i] = 0.0;
    }
    for (int i = 0; i < nweights; i++) {
        code_to_prob[hw_to_code[i]] += sr_binomial_pdf(i, nweights - 1, 0.5);
    }
}


static inline unsigned short get_wlong_hamming_weight(GeneratorState *obj,
    unsigned int values_per_word)
{
    unsigned short hw = 0;
    for (unsigned int i = 0; i < values_per_word; i++) {
        hw += get_uint64_hamming_weight(obj->gi->get_bits(obj->state));
    }
    return hw;
}

/**
 * @brief This version of `hamming_ot_test` tries to find longer-range
 * correlations by computation of Hamming weights for 256-bit words.
 * @details It can be considered as a modification of DC6-9x1Bytes-1 test
 * from PractRand by Chris Doty-Humphrey. But it also resembles BCFN test
 * from the same program and z9 test from gjrand by G. Jones.
 *
 * Detects additive/subtractive lagged Fibonacci with small lags; its
 * strong side is detection or RANROT32 generator: it is an additive
 * lagged Fibonacci PRNG that uses rotations to bypass birthday spacings
 * and gap tests.
 * 
 */
TestResults hamming_ot_long_test(GeneratorState *obj, const HammingOtLongOptions *opts)
{
    const unsigned int bits_per_word = opts->wordsize;
    double code_to_prob[4];
    unsigned short *hw_to_code = calloc(bits_per_word + 1, sizeof(unsigned short));
    if (hw_to_code == NULL) {
        fprintf(stderr, "***** hamming_ot_long_test: not enough memory *****\n");
        exit(EXIT_FAILURE);
    }
    hamming_ot_long_fill_hw_tables(hw_to_code, code_to_prob, bits_per_word);
    // Parameters for 18-bit tuple with 2-bit digits
    static const unsigned int code_nbits = 2, tuple_size = 9;
    const uint64_t tuple_mask = (1ull << code_nbits * tuple_size) - 1;
    //
    HammingTuplesTable table;
    HammingTuplesTable_init(&table, code_nbits, tuple_size, code_to_prob);

    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    unsigned short cur_weight; // Current Hamming weight
    unsigned int values_per_word = bits_per_word / obj->gi->nbits;
    unsigned long long ntuples = opts->nvalues / values_per_word;
    obj->intf->printf("Hamming weights based test (overlapping tuples), long version\n");
    obj->intf->printf("  Sample size, values:     %llu (2^%.2f or 10^%.2f)\n",
        opts->nvalues, sr_log2((double) opts->nvalues), log10((double) opts->nvalues));
    obj->intf->printf("  Word size, bits:         %u\n", bits_per_word);
    obj->intf->printf("  Used probabilities for codes:\n");
    for (int i = 0; i < 4; i++) {
        obj->intf->printf("    p(%d) = %10.8f\n", i, code_to_prob[i]);
    }
    // Process input as a sequence of 256-bit words
    // Pre-fill tuple
    cur_weight = get_wlong_hamming_weight(obj, values_per_word);
    for (unsigned int i = 0; i < tuple_size; i++) {
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = get_wlong_hamming_weight(obj, values_per_word);
    }
    // Generate other overlapping tuples
    for (unsigned long long i = 0; i < ntuples; i++) {
        // Process current tuple
        table.tuples[tuple].count++;
        // Update tuple
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = get_wlong_hamming_weight(obj, values_per_word);
    }
    // Convert tuples counters to the test results (p-value etc.)
    obj->intf->printf("   Number of tuples types: %llu\n",
        (unsigned long long) table.len);
    TestResults res = HammingTuplesTable_get_results(&table);
    obj->intf->printf("   Number of bins after reduction: %llu\n",
        (unsigned long long) table.len);
    obj->intf->printf("  zemp = %g, p = %g\n", res.x, res.p);
    obj->intf->printf("\n");
    HammingTuplesTable_destruct(&table);
    free(hw_to_code);
    return res;
}


double hamming_distr_calc_zemp(unsigned long long *o, size_t nbits)
{
    static const double NORM_STD = 1.3; // Empirical standard deviation
    double chi2emp = 0.0;
    unsigned long long npoints = 0;
    unsigned long df = 0;
    for (size_t i = 0; i < nbits; i++) {
        npoints += o[i];
    }
    double *p = calloc(nbits + 1, sizeof(double));
    sr_binomial_pdf_all(p, nbits, 0.5);
    for (size_t i = 0; i < nbits; i++) {
        double e_i = npoints * p[i];
        if (e_i > 25.0) {
            chi2emp += pow(o[i] - e_i, 2.0) / e_i;
            df++;
        }
    }
    free(p);
    return sr_chi2_to_stdnorm_approx(chi2emp, df) / NORM_STD;
}

/**
 * @brief Keeps empirical frequencies (histograms), empirical random values
 * obtained from the chi2 criterion and the corresponding p-values for the
 * `hamming_distr` statistical test.
 */
typedef struct {
    unsigned long long *o; ///< Frequencies for Hamming weights of blocks.
    unsigned long long *o_xor; ///< Frequencies for Hamming weights of blocks pairwise XORs.
    size_t nbits; ///< Number of bits in the block.
    double z; ///< chi2 test result (standard normal distribution) for `o`.
    double p; ///< p-value obtained from `z`.
    double z_xor; ///< chi2 test result (standard normal distribution) for `o_xor`.
    double p_xor; ///< p-value obtained from `p_xor`.
} HammingDistrHist;

/**
 * @brief Initializes the HammingDistrHist struct.
 * @param obj    Pointer to the structure to be initialized.
 * @param nbits  Number of bits per block.
 * @memberof HammingDistrHist
 */
static void HammingDistrHist_init(HammingDistrHist *obj, size_t nbits)
{
    obj->o     = calloc(nbits + 1, sizeof(unsigned long long));
    obj->o_xor = calloc(nbits + 1, sizeof(unsigned long long));
    obj->nbits = nbits;
    obj->z = NAN;     obj->p = NAN;
    obj->z_xor = NAN; obj->p_xor = NAN;
}

/**
 * @brief Calculate all statistics for histograms/frequencies kept
 * inside the structure.
 * @memberof HammingDistrHist
 */
static void HammingDistrHist_calc_stats(HammingDistrHist *obj)
{
    obj->z = hamming_distr_calc_zemp(obj->o, obj->nbits);
    obj->p = sr_stdnorm_pvalue(obj->z);
    obj->z_xor = hamming_distr_calc_zemp(obj->o_xor, obj->nbits);
    obj->p_xor = sr_stdnorm_pvalue(obj->z_xor);
}


static void HammingDistrHist_destruct(HammingDistrHist *obj)
{
    free(obj->o); obj->o = NULL;
    free(obj->o_xor); obj->o_xor = NULL;
}


static inline void calc_block_hw_sums(unsigned long long *hw_freq,
    const int *hw_vals, int block_len, int total_len)
{
    for (int i = 0; i < total_len; i += block_len) {
        int hw_sum = 0;
        for (int j = 0; j < block_len; j++) {
            hw_sum += hw_vals[i + j];
        }
        hw_freq[hw_sum]++;
    }
}

static inline void calc_block_hw_xorsums(unsigned long long *hw_freq,
    const uint64_t *x, int block_len, int total_len)
{
    for (int i = 0; i < total_len; i += block_len * 2) {
        int hw_xorsum = 0;
        for (int j = 0; j < block_len; j++) {
            uint64_t xored = x[i + j] ^ x[i + j + block_len];
            hw_xorsum += get_uint64_hamming_weight(xored);
        }
        hw_freq[hw_xorsum]++;
    }
}

/**
 * @brief This test computes Hamming weight for bit blocks of a pseudorandom
 * sequence and compares histograms (empirical distributions) with theoretical
 * ones.
 * @details Two histograms (empirical distributions) are constructed:
 *
 * 1. Histogram of Hamming weights of values.
 * 2. Histogram of XORed Hamming weights of values. I.e. Hamming weights of
 *    x1 ^ x2, x3 ^ x4, x5 ^ x6 are processed.
 *
 * The x values are bit blocks, i.e. 1, 2, 4 or 8 concatenated pseudorandom
 * values.
 *
 * The variant 2 (XORed) allows to catch 32-bit LCGs such as `lcg69069` or
 * `randu`; also detects SplitMix with some bad gammas such as 1. The idea of
 * the test is taken from avalanche effect estimation methods. Probably this
 * test may be extended by XORing more distant values.
 *
 * It seems that this test can detect PRNG that exceeded its period during
 * the testing (e.g. `splitmix32`). The test also detects some LFSRs with
 * small sparse matrices (such as SHR3, XSH, xorshift128, lrnd64_255) and
 * some obsolete combined generators such as SuperDuper73.
 */
TestResults hamming_distr_test(GeneratorState *obj, const HammingDistrOptions *opts)
{
    TestResults ans = TestResults_create("hamming_distr");
    size_t nbits = obj->gi->nbits;
    obj->intf->printf("Hamming weights distribution test (histogram)\n");
    if (opts->nlevels < 1) {
        obj->intf->printf("  Invalid nlevels value\n");
        return ans;
    }
    int block_len = 1 << opts->nlevels;
    HammingDistrHist *h = calloc(opts->nlevels, sizeof(HammingDistrHist));
    uint64_t *x = calloc(block_len, sizeof(uint64_t));
    int *hw = calloc(block_len, sizeof(int));
    for (int i = 0; i < opts->nlevels; i++) {
        HammingDistrHist_init(&h[i], nbits << i);
    }
    obj->intf->printf("  Sample size, values:     %llu (2^%.2f or 10^%.2f)\n",
        opts->nvalues, sr_log2((double) opts->nvalues), log10((double) opts->nvalues));
    uint64_t mask = (nbits == 64) ? 0xFFFFFFFFFFFFFFFF : ((1ull << nbits) - 1);
    uint64_t not_mask = ~mask;
    uint64_t bad_or = 0;
    for (unsigned long long i = 0; i < opts->nvalues; i += block_len) {
        for (int j = 0; j < block_len; j++) {
            uint64_t u = obj->gi->get_bits(obj->state);
            x[j] = u & mask;
            bad_or |= u & not_mask;
            hw[j] = get_uint64_hamming_weight(x[j]);
        }
        // 1-value blocks
        for (int j = 0; j < block_len; j += 2) {
            h[0].o[hw[j]]++; h[0].o[hw[j + 1]]++;
            h[0].o_xor[get_uint64_hamming_weight(x[j] ^ x[j + 1])]++;
        }
        // 2, 4, 8, 16 - value blocks
        for (int j = 1; j < opts->nlevels; j++) {
            calc_block_hw_sums(h[j].o,        hw, 1 << j, block_len);
            calc_block_hw_xorsums(h[j].o_xor, x,  1 << j, block_len);
        }
    }
    if (bad_or != 0) {
        obj->intf->printf("  Warning: generator output size exceeds its declared size\n");
    }
    double zabs_max = -1.0;
    obj->intf->printf("  Blocks analysis results\n");
    obj->intf->printf("    %8s | %8s %10s | %8s %10s\n",
        "bits", "z", "p", "z_xor", "p_xor");
    for (int i = 0; i < opts->nlevels; i++) {
        HammingDistrHist_calc_stats(&h[i]);
        obj->intf->printf("    %8d | %8.3f %10.3g | %8.3f %10.3g\n",
            (int) ((1 << i) * nbits), h[i].z, h[i].p, h[i].z_xor, h[i].p_xor);
        if (fabs(h[i].z) > zabs_max) {
            zabs_max = fabs(h[i].z); ans.p = h[i].p; ans.x = h[i].z;
        }
        if (fabs(h[i].z_xor) > zabs_max) {
            zabs_max = fabs(h[i].z_xor); ans.p = h[i].p_xor; ans.x = h[i].z_xor;
        }
    }
    ans.penalty = PENALTY_HAMMING_DISTR;
    ans.alpha = sr_stdnorm_cdf(ans.x);
    obj->intf->printf("  Final: z = %7.3f, p = %.3g\n", ans.x, ans.p);
    for (int i = 0; i < opts->nlevels; i++) {
        HammingDistrHist_destruct(&h[i]);
    }   
    free(h);
    free(x);
    free(hw);
    return ans;
}

///////////////////////////////////////////////
///// Interfaces (wrappers) for batteries /////
///////////////////////////////////////////////

TestResults hamming_ot_test_wrap(GeneratorState *obj, const void *udata)
{
    return hamming_ot_test(obj, udata);
}

TestResults hamming_ot_long_test_wrap(GeneratorState *obj, const void *udata)
{
    return hamming_ot_long_test(obj, udata);
}

TestResults hamming_distr_test_wrap(GeneratorState *obj, const void *udata)
{
    return hamming_distr_test(obj, udata);
}


