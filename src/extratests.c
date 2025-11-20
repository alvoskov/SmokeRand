/**
 * @file extratests.c
 * @brief Implementation of some statistical tests not included in the `brief`,
 * `default` and `full` batteries. These are 64-bit birthday paradox (not birthday
 * spacings!) test and 2D 16x16 Ising model tests.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/entropy.h"
#include "smokerand/extratests.h"
#include "smokerand/specfuncs.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

////////////////////////////////////////////////
///// BirthdayOptions class implementation /////
////////////////////////////////////////////////

static unsigned int birthday_get_nbits_per_value(const GeneratorInfo *gi)
{
    return (gi->nbits == 32) ? 64 : gi->nbits;
}

/**
 * @brief Fill birthday paradox test settings for the given
 * PRNG, sample length and lambda.
 * @details The number of collisions obeys Poisson distribution with
 * the next mathematical expectation:
 *
 * \f[
 * \lambda = \frac{\left(2^{L}\right)^2}{2\cdot 2^{b-e}}
 * \f]
 *
 * where \f$ 2^L \f$ is the sample lenth, \f$ b \f$ is the number of bits
 * in the PRNG output (usually 64), \f$ e \f$ is number of truncated (lower)
 * bits.
 */
static inline BirthdayOptions BirthdayOptions_create(const GeneratorInfo *gi,
    const unsigned int log2_len, const unsigned int log2_lambda)
{
    BirthdayOptions opts;
    const unsigned int nbits_per_value = birthday_get_nbits_per_value(gi);
    const unsigned int a = nbits_per_value + log2_lambda + 1;
    const unsigned int b = 2*log2_len;
    opts.e = (a >= b) ? (a - b) : 0;
    opts.n = 1ull << log2_len;
    return opts;
}

static inline double BirthdayOptions_calc_lambda(const BirthdayOptions *opts,
    const unsigned int nbits)
{
    double lambda = pow((double) opts->n, 2.0) / pow(2.0, (double) nbits - (double) opts->e + 1.0);
    return lambda;
}

///////////////////////////////////////////////
///// 64-bit birthday test implementation /////
///////////////////////////////////////////////

/**
 * @brief Generate truncated pseudorandom value
 */
static inline uint64_t birthday_gen_trvalue(GeneratorState *obj, uint64_t mask, int *is_ok)
{
    uint64_t u;
    long ctr = 0;
    static const long ctr_max = 100000000;
    if (obj->gi->nbits == 64) {
        do {            
            u = obj->gi->get_bits(obj->state);
            ctr++;
        } while ((u & mask) != 0 && ctr < ctr_max);
    } else {
        do {
            uint64_t lo = obj->gi->get_bits(obj->state);
            uint64_t hi = obj->gi->get_bits(obj->state);
            u = (hi << 32) | lo;
            ctr++;
        } while ((u & mask) != 0 && ctr < ctr_max);
    }
    *is_ok = ctr < ctr_max;
    return u;
}


/**
 * @brief Birthday paradox (not birthday spacings!) test for 64-bit pseudorandom
 * number generators. Detects 64-bit uniformly distributed PRNGS with 64-bit state.
 * @details This test for 64-bit PRNGs is suggested by M.E.O'Neill. It detects
 * the absence of duplicates in the output of 64-bit uniformly distributed generator
 * with 64-bit state.
 *
 * The number of duplicates is approximately Poisson distributed with the next
 * \f$ \lambda \f$ value:
 *
 * \f[
 * \lambda = \frac{n^2}{2\cdot 2^m}
 * \f]
 * where \f$ m = 2^{64} \f$. However even for \f$\lambda=8\f$ the \f$n=2^{34}\f$
 * that correspond to 128 GiB of RAM that is beyond capabilities of majority of
 * workstations in 2024. To overcome this problem the vast majority of PRNG output
 * is thrown out; only numbers with lower \f$ e \f$ bits equal to 0 are used.
 *
 * In the case of 32-bit generators it makes 64-integers by two subsequent calls
 * of the 32-bit PRNG.
 *
 * References:
 *
 * 1. M.E. O'Neill. A Birthday Test: Quickly Failing Some Popular PRNGs
 *    https://www.pcg-random.org/posts/birthday-test.html
 */
TestResults birthday_test(GeneratorState *obj, const BirthdayOptions *opts)
{
    TestResults ans = TestResults_create("birthday");
    unsigned long long nvalues_raw = (opts->n << opts->e);
    unsigned int nbits_per_value = birthday_get_nbits_per_value(obj->gi);
    double lambda = BirthdayOptions_calc_lambda(opts, nbits_per_value);
    obj->intf->printf("  Sample size:      2^%.2f values (2^%.2f bytes)\n",
        sr_log2((double) opts->n), sr_log2(8.0 * (double) opts->n));
    if (opts->n < 8) {
        obj->intf->printf("  Sample size is too small");
        return ans;
    }
    obj->intf->printf("  Shift:            %d bits\n",   (int) opts->e);
    obj->intf->printf("  Raw sample size:  2^%.2f values (2^%.2f bytes)\n",
        sr_log2((double) nvalues_raw), sr_log2(8.0 * (double) nvalues_raw));
    obj->intf->printf("  lambda = %g\n", lambda);
    obj->intf->printf("  Filling the array with 'birthdays'\n");
    uint64_t mask = (1ull << opts->e) - 1;
    time_t tic = time(NULL);
    uint64_t cpu_tic = cpuclock();
    uint64_t *x = calloc((size_t) opts->n, sizeof(uint64_t));
    if (x == NULL) {
        obj->intf->printf("  Not enough memory (2^%.0f bytes is required)\n",
            sr_log2((double) opts->n * 8.0));
        return ans;
    }
    unsigned long long bytes_per_trvalue = 1ull << (opts->e + 3);
    for (unsigned long long i = 0; i < opts->n; i++) {
        int is_ok;
        x[i] = birthday_gen_trvalue(obj, mask, &is_ok);
        if (!is_ok) {
            obj->intf->printf("  The generator is too flawed to return a truncated value\n");
            ans.x = 0;
            ans.p = sr_poisson_cdf(ans.x, lambda);
            ans.alpha = sr_poisson_pvalue(ans.x, lambda);
            free(x);
            return ans;
        }
        if (i % (opts->n / 1000) == 0) {
            unsigned long nseconds_total, nseconds_left;
            double mib_per_sec, cpb;
            uint64_t cpu_toc = cpuclock();
            if (cpu_toc < cpu_tic) {
                cpu_tic = cpu_toc;
            }
            cpb = (double) (cpu_toc - cpu_tic) / (double) (i * bytes_per_trvalue);
            nseconds_total = (unsigned long) (time(NULL) - tic);
            nseconds_left = (unsigned long) ( ((unsigned long long) nseconds_total * (opts->n - i)) / (i + 1) );
            mib_per_sec = (double) (i * bytes_per_trvalue) / (double) nseconds_total / (1ul << 20);
            obj->intf->printf("\r    %.1f %% completed; ",
                100.0 * (double) i / (double) opts->n);
            obj->intf->printf("time elapsed: "); print_elapsed_time(nseconds_total);
            obj->intf->printf(", left: ");
            print_elapsed_time(nseconds_left);
            if (mib_per_sec > 1024.0) {
                obj->intf->printf("; %.2f GiB/s (%.3g cpb)",
                    mib_per_sec / 1024.0, cpb);
            } else if (mib_per_sec == mib_per_sec && mib_per_sec < 1e100) {
                obj->intf->printf("; %.1f MiB/s (%.3g cpb)",
                    mib_per_sec, cpb);
            }
            obj->intf->printf("    ");
        }
    }
    // Frequencies analysis
    obj->intf->printf("\n");
    obj->intf->printf("\n  Sorting the array\n");
    // qsort is used instead of radix sort to prevent "out of memory" error:
    // 2^30 of u64 is 8GiB of data
    tic = time(NULL);
    quicksort64(x, (size_t) opts->n); // Not radix: to prevent "out of memory"
    obj->intf->printf("  Time elapsed: ");
    print_elapsed_time((unsigned long long) (time(NULL) - tic));
    obj->intf->printf("\n");    
    obj->intf->printf("  Searching duplicates\n");
    unsigned long ndups = 0;
    for (size_t i = 0; i < opts->n - 1; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    ans.x = (double) ndups;
    ans.p = sr_poisson_cdf(ans.x, lambda);
    ans.alpha = sr_poisson_pvalue(ans.x, lambda);
    obj->intf->printf("  x = %g (ndups); p = %g; 1-p=%g\n", ans.x, ans.p, ans.alpha);
    free(x);
    return ans;
}

/**
 * @brief Estimates an optimal sample size for the birthday paradox test.
 * @details The optimal size is about half of available physical RAM,
 * the higher RAM consumption means higher performance of the test. If
 * an information about RAM volume is not available then the next default
 * settings are used:
 *
 * - 64-bit platforms: 2^30 (requires 8 GiB of RAM and ~30min)
 * - 32-bit platforms: 2^27 (requires 1 GiB of RAM, very slow)
 *
 */
static unsigned int birthday_get_log2_n(const CallerAPI *intf)
{
    RamInfo raminfo;
    int ans = intf->get_ram_info(&raminfo);
    if (ans == 0 || raminfo.phys_total_nbytes == RAM_SIZE_UNKNOWN) {
        return (SIZE_MAX == UINT64_MAX) ? 30 : 27;
    } else {
        const double log2_ramsize = sr_log2((double) raminfo.phys_total_nbytes);
        int log2_n = (int) floor(log2_ramsize);
        log2_n += (log2_ramsize - log2_n > 0.5) ? 1 : 0;
        log2_n -= 4; // Half of all RAM, also divide by 2^3 (nbytes->n_u64)
        if (log2_n < 19) {
            log2_n = 19;
        }
        return (unsigned int) log2_n;
    }
}

/**
 * @brief An implementation of birthday paradox test for 64-bit PRNGS
 * with adaptive selection of lambda.
 * @details It consumes a lot of RAM and sample size differs for
 * different platforms:
 *
 * - 64-bit platform: 2^30 (requires 8 GiB of RAM and ~30min)
 * - 32-bit platform: 2^27 (requires 1 GiB of RAM, very slow)
 *
 * The test consists of two stages:
 *
 * - Small subtest: \f$ \lambda = 4\f$, usually takes less than 10 min.
 *   If no duplicates are found - the more sensitive "large" subtest
 *   is launched. If they are found - p-value is calculated.
 * - Large subtest: \f$ \lambda = 16\f$, usually takes less than 1 hour.
 *   Then p-value for sum of duplicates from "small" and "large" subtests
 *   is calculated.
 */
BatteryExitCode battery_birthday(const GeneratorInfo *gen, const CallerAPI *intf)
{
    const unsigned int log2_n = birthday_get_log2_n(intf);
    const unsigned int nbits_per_value = birthday_get_nbits_per_value(gen);
    BirthdayOptions opts_small = BirthdayOptions_create(gen, log2_n, 2);
    BirthdayOptions opts_large = BirthdayOptions_create(gen, log2_n, 4);
    intf->printf("64-bit birthday paradox test\n");
    if (gen->nbits != 64) {
        intf->printf("  Output from 32-bit generator: concatenation will be used\n");
    }
    GeneratorState obj = GeneratorState_create(gen, intf);
    TestResults ans = birthday_test(&obj, &opts_small);
    if (ans.x == 0) {
        double x_small = ans.x;
        double lambda = BirthdayOptions_calc_lambda(&opts_small, nbits_per_value) +
            BirthdayOptions_calc_lambda(&opts_large, nbits_per_value);
        intf->printf("  No duplicates found: more sensitive test required\n");
        intf->printf("  Running the variant with larger lambda\n");
        ans = birthday_test(&obj, &opts_large);
        intf->printf("  p-value for x1 + x2 (lambda = %g):\n", lambda);
        ans.x += x_small;
        ans.p = sr_poisson_cdf(ans.x, lambda);
        ans.alpha = sr_poisson_pvalue(ans.x, lambda);
        intf->printf("  x = %g (ndups); p = %g; 1-p=%g\n", ans.x, ans.p, ans.alpha);
    }
    GeneratorState_destruct(&obj);
    if (ans.p < 1e-6 || ans.p > 1.0 - 1e-6) {
        return BATTERY_FAILED;
    } else {
        return BATTERY_PASSED;
    }
}

/////////////////////////////////
///// Blocks frequency test /////
/////////////////////////////////

typedef struct {
    unsigned long long *bytefreq;
    unsigned long long *w16freq;
    unsigned long long nbytes;
    unsigned long long nw16;
} BlockFrequency;

void BlockFrequency_init(BlockFrequency *obj)
{
    obj->bytefreq = calloc(256, sizeof(unsigned long long));
    obj->w16freq = calloc(65536, sizeof(unsigned long long));
    obj->nbytes = 0;
    obj->nw16 = 0;
}

void BlockFrequency_destruct(BlockFrequency *obj)
{
    free(obj->bytefreq);
    free(obj->w16freq);
}

static inline void BlockFrequency_count(BlockFrequency *obj,
    const uint64_t u, const unsigned int nbytes)
{
    uint64_t tmp = u;
    // Bytes counter
    for (unsigned int i = 0; i < nbytes; i++) {
        obj->bytefreq[tmp & 0xFF]++;
        tmp >>= 8;
    }
    obj->nbytes += nbytes;
    // 16-bit chunks counter
    tmp = u;
    for (unsigned int i = 0; i < nbytes / 2; i++) {
        obj->w16freq[tmp & 0xFFFF]++;
        tmp >>= 16;
    }    
    obj->nw16 += nbytes / 2;
}


int BlockFrequency_calc(BlockFrequency *obj)
{
    const double pcrit = 1.0e-10;
    const double pcrit_bytes = pcrit / 256.0, pcrit_w16 = pcrit / 65536.0;
    double chi2_bytes = 0.0, chi2_w16 = 0.0;
    double zmax_bytes = 0.0, zmax_w16 = 0.0;
    int zmax_bytes_ind = -1;
    long zmax_w16_ind = -1;
    for (int i = 0; i < 256; i++) {        
        long long Ei = (long long) obj->nbytes / 256;
        long long dE = (long long) obj->bytefreq[i] - (long long) Ei;
        chi2_bytes += pow((double) dE, 2.0) / (double) Ei;
        double z_bytes = fabs((double) dE) / sqrt( (double) obj->nbytes * 255.0 / 65536.0);
        if (zmax_bytes < z_bytes) {
            zmax_bytes = z_bytes;
            zmax_bytes_ind = i;
        }
    }
    for (long i = 0; i < 65536; i++) {
        long long Ei = (long long) obj->nw16 / 65536;
        long long dE = (long long) obj->w16freq[i] - (long long) Ei;
        chi2_w16 += pow((double) dE, 2.0) / (double) Ei;
        double z_w16 = fabs((double) dE) /
            sqrt( (double) obj->nw16 * 65535.0 / 65536.0 / 65536.0);
        if (zmax_w16 < z_w16) {
            zmax_w16 = z_w16;
            zmax_w16_ind = i;
        }
    }
    double p_bytes = sr_halfnormal_pvalue(zmax_bytes);
    double p_w16 = sr_halfnormal_pvalue(zmax_w16);
    printf("2^%g bytes analyzed\n", sr_log2((double) obj->nbytes));
    printf("  %10s %10s %10s %10s %10s %10s %10s\n",
        "Chunk", "chi2emp", "p(chi2)", "zmax", "max_ind", "p(zmax)", "p(crit)");
    printf("  %10s %10g %10.2g %10.3g %10d %10.2g %10.2g\n",
        "8 bits", chi2_bytes, sr_chi2_pvalue(chi2_bytes, 255),
        zmax_bytes, zmax_bytes_ind, p_bytes, pcrit_bytes);
    printf("  %10s %10g %10.2g %10.3g %10ld %10.2g %10.2g\n",
        "16 bits", chi2_w16, sr_chi2_pvalue(chi2_w16, 65536),
        zmax_w16, zmax_w16_ind, p_w16, pcrit_w16);
    // p-values interpretation
    if (p_bytes < pcrit_bytes) {
        printf("===== zmax_bytes test failed =====\n");
        return 0;
    } else if (p_w16 < pcrit_w16) {
        printf("===== zmax_w16 test failed =====\n");
        return 0;
    } else {
        return 1;
    }
}

BatteryExitCode battery_blockfreq(const GeneratorInfo *gen, const CallerAPI *intf)
{
    BlockFrequency freq;
    BlockFrequency_init(&freq);
    GeneratorState obj = GeneratorState_create(gen, intf);
    size_t block_size = 1U << 30;
    time_t tic = time(NULL);
    int is_ok = 1;
    while (is_ok) {
        if (gen->nbits == 64) {
            for (size_t i = 0; i < block_size; i++) {
                uint64_t u = obj.gi->get_bits(obj.state);
                BlockFrequency_count(&freq, u, 8);
            }
        } else {
            for (size_t i = 0; i < block_size; i++) {
                uint64_t u = obj.gi->get_bits(obj.state);
                BlockFrequency_count(&freq, u, 4);
            }
        }
        is_ok = BlockFrequency_calc(&freq);
        printf("  Time elapsed: ");
        print_elapsed_time((unsigned long long) (time(NULL) - tic));
        printf("\n\n");
    }
    BlockFrequency_destruct(&freq);
    GeneratorState_destruct(&obj);
    return (is_ok) ? BATTERY_PASSED : BATTERY_FAILED;
}

/////////////////////////////////////
///// 2D Ising model based test /////
/////////////////////////////////////

/**
 * @brief Lattice cell neighbours description for 2D Ising model.
 */
typedef struct {
    unsigned int inds[4];
} Neighbours2D;

/**
 * @brief Two-dimensional lattice for 2D Ising model.
 */
typedef struct {
    unsigned int L; ///< Lattice size
    unsigned int N; ///< Number of elements, LxL
    int8_t *s; ///< Spins (-1 or +1)
    Neighbours2D *nn; ///< Neigbours
} Ising2DLattice;


static inline unsigned int inc_modL(unsigned int i, unsigned int L)
{
    return (++i) % L;
}

static inline unsigned int dec_modL(unsigned int i, unsigned int L)
{
    return (i > 0) ? (i - 1) : (L - 1);
}

void Ising2DLattice_init(Ising2DLattice *obj, unsigned int L)
{
    obj->L = L;
    obj->N = L * L;
    obj->s = calloc(obj->N, sizeof(int8_t));
    obj->nn = calloc(obj->N, sizeof(Neighbours2D));
    if (obj->s == NULL || obj->nn == NULL) {
        fprintf(stderr, "***** Ising2DLattice_init: not enough memory *****");
        free(obj->s); free(obj->nn);
        exit(EXIT_FAILURE);
    }
    // Precalculate neighbours indexes
    for (unsigned int i = 0; i < obj->N; i++) {
        const unsigned int ix = i % L, iy = i / L;
        //printf("%d %d | ", ix, iy);
        obj->s[i] = +1;
        obj->nn[i].inds[0] = iy * L + inc_modL(ix, L);
        obj->nn[i].inds[1] = iy * L + dec_modL(ix, L);
        obj->nn[i].inds[2] = inc_modL(iy, L) * L + ix;
        obj->nn[i].inds[3] = dec_modL(iy, L) * L + ix;
    }
}

void Ising2DLattice_print(Ising2DLattice *obj)
{
    for (size_t i = 0, pos = 0; i < obj->L; i++) {
        for (size_t j = 0; j < obj->L; j++) {
            printf("%2d ", (int) obj->s[pos++]);
        }
        printf("\n");
    }
}

void Ising2DLattice_destruct(Ising2DLattice *obj)
{
    free(obj->s); obj->s = NULL;
    free(obj->nn); obj->nn = NULL;    
}

/**
 * @brief Calculate energy for 2D lattice in 2D Ising model.
 */
int Ising2DLattice_calc_energy(const Ising2DLattice *obj)
{
    int energy = 0;
    for (unsigned int i = 0; i < obj->N; i++) {
        energy += (obj->s[i] == obj->s[obj->nn[i].inds[0]]) ? 1 : -1;
        energy += (obj->s[i] == obj->s[obj->nn[i].inds[2]]) ? 1 : -1;
    }
    return energy;
}


static void Ising2DLattice_flip_wolff_internal(Ising2DLattice *obj, size_t ind, int8_t s0,
    GeneratorState *gs, const uint64_t p_int)
{
    // Flip spin
    obj->s[ind] = (int8_t) (-s0);
    // Go to neighbours
    for (size_t i = 0; i < 4; i++) {
        size_t nn_ind = obj->nn[ind].inds[i];
        uint64_t rnd = gs->gi->get_bits(gs->state);
        if (obj->s[nn_ind] == s0 && rnd <= p_int) {
            Ising2DLattice_flip_wolff_internal(obj, nn_ind, s0, gs, p_int);
        }
    }    
}

/**
 * @brief Wolff algorithm: one flip based on recursive algorithm.
 * @details The next probability of flip is used:
 *
 * \f[
 * p = 1 - exp(-2j_c) \approx 0.585786
 * \f]
 *
 * \f[
 * j_c = \frac{1}{2} \ln\left(1 + \sqrt{2}\right)
 * \f]
 */
void Ising2DLattice_flip_wolff(Ising2DLattice *obj, size_t ind, GeneratorState *gs)
{
    const uint64_t p_int = (gs->gi->nbits == 32) ? 2515933592ull : 10805852496753538807ull; // p * 2^nbits
    Ising2DLattice_flip_wolff_internal(obj, ind, obj->s[ind], gs, p_int);
}

static inline size_t
Ising2DLattice_rand_index(Ising2DLattice *obj, GeneratorState *gs)
{
    return (size_t) (gs->gi->get_bits(gs->state) % obj->N);
}

/**
 * @brief Metropolis algorithm: one pass consisting of L^2 flips. Catches
 * 32-bit linear congruental generators like '69069'. Rather slow.
 */
void Ising2DLattice_pass_metropolis(Ising2DLattice *obj, GeneratorState *gs)
{
    const double jc = log(1 + sqrt(2)) / 2;
    const double p_mul = pow(2.0, gs->gi->nbits);
    uint64_t n_same_to_p_int[5];
    for (int i = 2; i <= 4; i++) {
        const double dE = (i - 2) * 4;
        n_same_to_p_int[i] = (uint64_t) (exp(-dE * jc) * p_mul);
    }

    for (size_t ii = 0; ii < obj->N; ii++) {
        // i index computation procedure is essential for test sensitivity
        const size_t i = Ising2DLattice_rand_index(obj, gs);
        int n_same = 0;
        for (size_t j = 0; j < 4; j++) {
            if (obj->s[obj->nn[i].inds[j]] == obj->s[i])
                n_same++;
        }
        const double dE = (n_same - 2) * 4;
        if (dE < 0) {
            obj->s[i] = (int8_t) (-obj->s[i]);
        } else {
            if (n_same_to_p_int[n_same] > gs->gi->get_bits(gs->state)) {
                obj->s[i] = (int8_t) (-obj->s[i]);
            }
        }
    }
}

void Ising2DLattice_pass(Ising2DLattice *obj, GeneratorState *gs, IsingAlgorithm alg)
{
    if (alg == ISING_WOLFF) {
        const size_t i = Ising2DLattice_rand_index(obj, gs);        
        Ising2DLattice_flip_wolff(obj, i, gs);
    } else if (alg == ISING_METROPOLIS) {
        Ising2DLattice_pass_metropolis(obj, gs);
    } else {
        fprintf(stderr, "Ising2DLattice_pass internal error: unknown algorithm\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief PRNG test based on Ising 2D model. It calculates internal energy
 * and heat capacity using Monte-Carlo method: Metropolis algorithm and Wolff
 * algorithm.
 * @details Two algorithms have a very different sensitivity:
 *
 * 1. Wolff algorithm detects flaws in SWB (subtract with borrow) and
 *    additive/subtractive lagged Fibonacci generators. But not in 32-bit
 *    linear congruental generators.
 * 2. Metropolis algorithm detects flaws in 32-bit LCG (e.g. in `69069`)
 *    but not in ALFIB or SWB.
 *
 * References:
 *
 * 1. Ferrenberg A.M., Landau D.P., Wong Y.J. Monte Carlo simulations: Hidden
 *    errors from 'good' random number generators // Phys. Rev. Lett. 1992.
 *    V. 69. N 23. P.3382-3384. https://doi.org/10.1103/PhysRevLett.69.3382
 * 2. Manssen M., Weigel M., Hartmann, A.K. Random number generators for
 *    massively parallel simulations on GPU // Eur. Phys. J. Spec. Top. 2012.
 *    V. 210, P. 53-71 https://doi.org/10.1140/epjst/e2012-01637-8 
 * 3. Coddington P.D. Tests of random number generators using Ising model
 *    simulations // International Journal of Modern Physics C. 1996. V. 7.
 *    N 3. P. 295-303. https://doi.org/10.1142/S0129183196000235
 */
TestResults ising2d_test(GeneratorState *gs, const Ising2DOptions *opts)
{
    static const double e_ref = 1.4530649029;
    static const double cv_ref = 1.4987048885;
    const double jc = log(1 + sqrt(2)) / 2;
    Ising2DLattice obj;
    Ising2DLattice_init(&obj, 16);
    TestResults res = TestResults_create("ising2d");
    if (opts->algorithm == ISING_WOLFF) {
        res.name = "ising2d_wolff";
    } else if (opts->algorithm == ISING_METROPOLIS) {
        res.name = "ising2d_metropolis";
    } else {
        res.name = "ising2d_unknown";
        return res;
    }
    gs->intf->printf("Ising 2D model test\n");
    gs->intf->printf("  L = %d, algorithm = %s, sample_len = %lu, nsamples = %u\n",
        (int) obj.L, res.name, opts->sample_len, opts->nsamples);
    // Warm-up
    for (unsigned int i = 0; i < opts->sample_len; i++) {
        Ising2DLattice_pass(&obj, gs, opts->algorithm);
    }
    // Sampling
    double *e = calloc(opts->nsamples, sizeof(double));
    double *cv = calloc(opts->nsamples, sizeof(double));
    if (e == NULL || cv == NULL) {
        fprintf(stderr, "***** ising2d_test: not enough memory *****\n");
        free(e); free(cv);
        exit(EXIT_FAILURE);
    }
    for (unsigned long ii = 0; ii < opts->nsamples; ii++) {
        long long energy_sum = 0, energy_sum2 = 0;
        for (unsigned int i = 0; i < opts->sample_len; i++) {
            Ising2DLattice_pass(&obj, gs, opts->algorithm);
            int energy = Ising2DLattice_calc_energy(&obj);
            energy_sum += energy;
            energy_sum2 += energy * energy;
        }
        double Emean = (double) energy_sum / (double) opts->sample_len;
        double E2mean = (double) energy_sum2 / (double) opts->sample_len;
        e[ii] = Emean / obj.N;
        cv[ii] = (E2mean - Emean * Emean) / obj.N * jc * jc;
        gs->intf->printf("%2lu of %2lu: e = %12.8f, cv = %12.8f\n",
            ii + 1, opts->nsamples, e[ii], cv[ii]);
    }
    // Mean and std
    double e_mean = 0.0, e_std = 0.0, cv_mean = 0.0, cv_std = 0.0;
    for (unsigned int i = 0; i < opts->nsamples; i++) {
        e_mean += e[i];
        cv_mean += cv[i];
    }
    e_mean /= opts->nsamples;
    cv_mean /= opts->nsamples;
    for (unsigned int i = 0; i < opts->nsamples; i++) {
        e_std += pow(e[i] - e_mean, 2.0);
        cv_std += pow(cv[i] - cv_mean, 2.0);
    }
    e_std = sqrt(e_std / (opts->nsamples - 1));
    cv_std = sqrt(cv_std / (opts->nsamples - 1));
    double e_z = (e_mean - e_ref) / (e_std / sqrt(opts->nsamples));
    double cv_z = (cv_mean - cv_ref) / (cv_std / sqrt(opts->nsamples));
    unsigned long df = opts->nsamples - 1;
    gs->intf->printf("e_mean  = %12.8g; e_std  = %12.8g; z = %12.8g; p = %.3g\n",
        e_mean, e_std, e_z, sr_t_pvalue(e_z, df));
    gs->intf->printf("cv_mean = %12.8g; cv_std = %12.8g; z = %12.8g; p = %.3g\n",
        cv_mean, cv_std, cv_z, sr_t_pvalue(cv_z, df));
    free(e);
    free(cv);
    // Ising2DLattice_print(&obj);
    Ising2DLattice_destruct(&obj);
    // Fill results
    res.penalty = PENALTY_ISING2D;
    if (fabs(cv_z) > fabs(e_z)) {
        res.x = cv_z;
        res.p = sr_t_pvalue(cv_z, df);
        res.alpha = sr_t_cdf(cv_z, df);
    } else {
        res.x = e_z;
        res.p = sr_t_pvalue(e_z, df);
        res.alpha = sr_t_cdf(e_z, df);
    }
    return res;
}


//////////////////////////////////////
///// Volume of unit sphere test /////
//////////////////////////////////////

/**
 * @brief Calculate volume of the n-dimensional (hyper)sphere part situated
 * inside the [0;1]^n n-dimensional hypercube.
 */
static double calc_usphere_volplus(unsigned int ndims)
{
    double n_half = (double) ndims / 2.0;
    return exp(n_half * log(M_PI) - lgamma(1.0 + n_half) - ndims*log(2.0));
}

/**
 * @brief This test is based on computation of n-dimensional (n >= 2)
 * (hyper)sphere volume by means of Monte-Carlo method.
 * @details This test is not very sensitive and catches only some rare low-grade
 * generators, e.g. RANDU and shr3 (xorshift32). It is useful for educational
 * purposes and as a component of performance benchmarks.
 */
TestResults unit_sphere_volume_test(GeneratorState *gs, const UnitSphereOptions *opts)
{
    const double tof_coeff = pow(2.0, -((int) gs->gi->nbits));
    long long n_inside = 0;
    TestResults ans = TestResults_create("usphere");
    if (opts->ndims < 2 || opts->ndims > 20 || opts->npoints < 1000) {
        return ans;
    }
    gs->intf->printf("UnitSphereVolume test\n");
    gs->intf->printf("  ndims = %u; nvalues = %llu (10^%.2f or 2^%.2f)\n",
        opts->ndims, opts->npoints,
        log10((double) opts->npoints),
        sr_log2((double) opts->npoints));
    for (unsigned long long i = 0; i < opts->npoints; i++) {
        double d = 0.0;
        for (unsigned int j = 0; j < opts->ndims; j++) {
            const double x = (double) gs->gi->get_bits(gs->state) * tof_coeff;
            d += x * x;
        }
        if (d <= 1.0) {
            n_inside++;
        }
    }
    const double v_theor = calc_usphere_volplus(opts->ndims);
    const double v_num = (double) n_inside / (double) opts->npoints;
    const double s_theor = sqrt((double) opts->npoints * v_theor * (1.0 - v_theor));
    const long long n_inside_theor = (long long) (v_theor * (double) opts->npoints);
    ans.x = (double) (n_inside - n_inside_theor) / s_theor;
    ans.p = sr_stdnorm_pvalue(ans.x);
    ans.alpha = sr_stdnorm_cdf(ans.x);
    gs->intf->printf("  v_num = %.10g, v_theor = %.10g\n", v_num, v_theor);
    gs->intf->printf("  z = %g, p = %g\n", ans.x, ans.p);
    return ans;
}

///////////////////////////////////////////////
///// Interfaces (wrappers) for batteries /////
///////////////////////////////////////////////

TestResults ising2d_test_wrap(GeneratorState *obj, const void *udata)
{
    return ising2d_test(obj, udata);
}

TestResults unit_sphere_volume_test_wrap(GeneratorState *gs, const void *udata)
{
    return unit_sphere_volume_test(gs, udata);
}


/////////////////////
///// Batteries /////
/////////////////////

BatteryExitCode battery_ising(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{
    static const Ising2DOptions
        metr  = {.sample_len = 5000000, .nsamples = 20, .algorithm = ISING_METROPOLIS},
        wolff = {.sample_len = 5000000, .nsamples = 20, .algorithm = ISING_WOLFF};
    
    static const TestDescription tests[] = {
        {"ising16_metropolis", ising2d_test_wrap, &metr},
        {"ising16_wolff",      ising2d_test_wrap, &wolff},
        {NULL, NULL, NULL}
    };
    const TestsBattery bat = {
        "ising", tests
    };
    if (gen != NULL) {
        return TestsBattery_run(&bat, gen, intf, opts);
    } else {
        TestsBattery_print_info(&bat);
        return BATTERY_PASSED;
    }
}


BatteryExitCode battery_unit_sphere_volume(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{
    static const UnitSphereOptions 
        usphere_2d  = {.ndims = 2,  .npoints = 2000000000},
        usphere_3d  = {.ndims = 3,  .npoints = 2000000000},
        usphere_4d  = {.ndims = 4,  .npoints = 2000000000},
        usphere_5d  = {.ndims = 5,  .npoints = 2000000000},
        usphere_6d  = {.ndims = 6,  .npoints = 2000000000},
        usphere_10d = {.ndims = 10, .npoints = 2000000000},
        usphere_12d = {.ndims = 12, .npoints = 2000000000},
        usphere_15d = {.ndims = 15, .npoints = 2000000000};


    static const TestDescription tests[] = {
        {"usphere_2d", unit_sphere_volume_test_wrap, &usphere_2d},
        {"usphere_3d", unit_sphere_volume_test_wrap, &usphere_3d},
        {"usphere_4d", unit_sphere_volume_test_wrap, &usphere_4d},
        {"usphere_5d", unit_sphere_volume_test_wrap, &usphere_5d},
        {"usphere_6d", unit_sphere_volume_test_wrap, &usphere_6d},
        {"usphere_10d", unit_sphere_volume_test_wrap, &usphere_10d},
        {"usphere_12d", unit_sphere_volume_test_wrap, &usphere_12d},
        {"usphere_15d", unit_sphere_volume_test_wrap, &usphere_15d},
        {NULL, NULL, NULL}
    };
    const TestsBattery bat = {
        "unitsphere", tests
    };
    if (gen != NULL) {
        return TestsBattery_run(&bat, gen, intf, opts);
    } else {
        TestsBattery_print_info(&bat);
        return BATTERY_PASSED;
    }
}
