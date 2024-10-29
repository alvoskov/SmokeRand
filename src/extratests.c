/**
 * @file extratests.c
 * @brief Implementation of some statistical tests not included in the `brief`,
 * `default` and `full` batteries. These are 64-bit birthday paradox (not birthday
 * spacings!) test and 2D 16x16 Ising model tests.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/extratests.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


///////////////////////////////////////////////
///// 64-bit birthday test implementation /////
///////////////////////////////////////////////

static int cmp_ints(const void *aptr, const void *bptr)
{
    uint64_t aval = *((uint64_t *) aptr), bval = *((uint64_t *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}


static inline double BirthdayOptions_calc_lambda(const BirthdayOptions *opts,
    const unsigned int nbits)
{
    double lambda = pow(opts->n, 2.0) / pow(2.0, nbits - opts->e + 1.0);
    return lambda;
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
 * where \f$ m = 2^{64} \f$. However even for $\lambda = 8$ the $n = 2^{34}$ that
 * correspond to 128GiB of RAM that is beyond capabilities of majority of
 * workstations in 2024. To overcome this problem the vast majority of PRNG output
 * is thrown out; only numbers with lower \f$ e \f$ bits equal to 0 are used.
 *
 * References:
 *
 * 1. M.E. O'Neill. A Birthday Test: Quickly Failing Some Popular PRNGs
 *    https://www.pcg-random.org/posts/birthday-test.html
 */
TestResults birthday_test(GeneratorState *obj, const BirthdayOptions *opts)
{
    TestResults ans = {.x = NAN, .p = NAN, .alpha = NAN};
    double lambda = BirthdayOptions_calc_lambda(opts, obj->gi->nbits);
    obj->intf->printf("  lambda = %g\n", lambda);
    obj->intf->printf("  Filling the array with 'birthdays'\n");
    uint64_t mask = (1ull << opts->e) - 1;
    time_t tic = time(NULL);
    uint64_t *x = calloc(opts->n, sizeof(uint64_t));
    if (x == NULL) {
        obj->intf->printf("  Not enough memory (2^%.0f bytes is required)\n",
            log2(opts->n * 8.0));
        return ans;
    }
    for (size_t i = 0; i < opts->n; i++) {
        uint64_t u;
        do {            
            u = obj->gi->get_bits(obj->state);
        } while ((u & mask) != 0);
        x[i] = u;
        if (i % (opts->n / 1000) == 0) {
            unsigned long nseconds_total, nseconds_left;            
            nseconds_total = time(NULL) - tic;
            nseconds_left = ((unsigned long long) nseconds_total * (opts->n - i)) / (i + 1);
            obj->intf->printf("\r    %.1f %% completed; ", 100.0 * i / (double) opts->n);
            obj->intf->printf("time elapsed: "); print_elapsed_time(nseconds_total);
            obj->intf->printf("; time left: ");
            print_elapsed_time(nseconds_left);
            obj->intf->printf("    ");
        }
    }
    // Frequencies analysis
    obj->intf->printf("\n");
    obj->intf->printf("\n  Sorting the array\n");
    // qsort is used instead of radix sort to prevent "out of memory" error:
    // 2^30 of u64 is 8GiB of data
    tic = time(NULL);
    qsort(x, opts->n, sizeof(uint64_t), cmp_ints); // Not radix: to prevent "out of memory"
    obj->intf->printf("  Time elapsed: ");
    print_elapsed_time(time(NULL) - tic);
    obj->intf->printf("\n");    
    obj->intf->printf("  Searching duplicates\n");
    unsigned long ndups = 0;
    for (size_t i = 0; i < opts->n - 1; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }
    ans.x = (double) ndups;
    ans.p = poisson_cdf(ans.x, lambda);
    ans.alpha = poisson_pvalue(ans.x, lambda);
    obj->intf->printf("  x = %g (ndups); p = %g; 1-p=%g\n", ans.x, ans.p, ans.alpha);
    free(x);
    return ans;
}

void battery_birthday(GeneratorInfo *gen, const CallerAPI *intf)
{
    size_t n = 1ull << 30;
    BirthdayOptions opts_small = {.n = n, .e = 7}; // lambda = 4
    BirthdayOptions opts_large = {.n = n, .e = 9}; // lambda = 20
    intf->printf("64-bit birthday paradox test\n");
    if (gen->nbits != 64) {
        intf->printf("  Error: the generator must return 64-bit values\n");
        return;
    }
    void *state = gen->create(intf);    
    GeneratorState obj = {.gi = gen, .state = state, .intf = intf};
    TestResults ans = birthday_test(&obj, &opts_small);
    if (ans.x == 0) {
        double x_small = ans.x;
        double lambda = BirthdayOptions_calc_lambda(&opts_small, gen->nbits) +
            BirthdayOptions_calc_lambda(&opts_large, gen->nbits);
        intf->printf("  No duplicates found: more sensitive test required\n");
        intf->printf("  Running the variant with larger lambda\n");
        ans = birthday_test(&obj, &opts_large);
        intf->printf("  p-value for x1 + x2 (lambda = %g):\n", lambda);
        ans.x += x_small;
        ans.p = poisson_cdf(ans.x, lambda);
        ans.alpha = poisson_pvalue(ans.x, lambda);
        intf->printf("  x = %g (ndups); p = %g; 1-p=%g\n", ans.x, ans.p, ans.alpha);
    }
    free(state);
    (void) ans;
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

void BlockFrequency_free(BlockFrequency *obj)
{
    free(obj->bytefreq);
    free(obj->w16freq);
}

static inline void BlockFrequency_count(BlockFrequency *obj,
    const uint64_t u, const int nbytes)
{
    uint64_t tmp = u;
    // Bytes counter
    for (int i = 0; i < nbytes; i++) {
        obj->bytefreq[tmp & 0xFF]++;
        tmp >>= 8;
    }
    obj->nbytes += nbytes;
    // 16-bit chunks counter
    tmp = u;
    for (int i = 0; i < nbytes / 2; i++) {
        obj->w16freq[tmp & 0xFFFF]++;
        tmp >>= 16;
    }    
    obj->nw16 += nbytes / 2;
}

static double halfnormal_pvalue(double x)
{
    return erfc(x / sqrt(2.0));
}

void BlockFrequency_calc(BlockFrequency *obj)
{
    double chi2_bytes = 0.0, chi2_w16 = 0.0;
    double zmax_bytes = 0.0, zmax_w16 = 0.0;
    double pcrit = 1.0e-10;
    for (size_t i = 0; i < 256; i++) {        
        long long Ei = (long long) obj->nbytes / 256;
        long long dE = (long long) obj->bytefreq[i] - (long long) Ei;
        chi2_bytes += pow(dE, 2.0) / (double) Ei;
        double z_bytes = fabs((double) dE) / sqrt( obj->nbytes * 255.0 / 65536.0);
        if (zmax_bytes < z_bytes) {
            zmax_bytes = z_bytes;
        }
    }
    for (size_t i = 0; i < 65536; i++) {
        long long Ei = (long long) obj->nw16 / 65536;
        long long dE = (long long) obj->w16freq[i] - (long long) Ei;
        chi2_w16 += pow(dE, 2.0) / (double) Ei;
        double z_w16 = fabs((double) dE) / sqrt( obj->nw16 * 65535.0 / 65536.0 / 65536.0);
        if (zmax_w16 < z_w16) {
            zmax_w16 = z_w16;
        }
    }
    printf("2^%g bytes analyzed\n", log2(obj->nbytes));
    printf("  %10s %10s %10s %10s %10s %10s\n",
        "Chunk", "chi2emp", "p(chi2)", "zmax", "p(zmax)", "p(crit)");
    printf("  %10s %10g %10.2g %10.3g %10.2g %10.2g\n",
        "8 bits", chi2_bytes, chi2_pvalue(chi2_bytes, 255.0),
        zmax_bytes, halfnormal_pvalue(zmax_bytes), pcrit / 256.0);
    printf("  %10s %10g %10.2g %10.3g %10.2g %10.2g\n",
        "16 bits", chi2_w16, chi2_pvalue(chi2_w16, 65536.0),
        zmax_w16, halfnormal_pvalue(zmax_w16), pcrit / 65536);
}

void battery_blockfreq(GeneratorInfo *gen, const CallerAPI *intf)
{
    BlockFrequency freq;
    BlockFrequency_init(&freq);
    void *state = gen->create(intf);
    size_t block_size = 1 << 30;
    time_t tic = time(NULL);
    while (1) {
        if (gen->nbits == 64) {
            for (size_t i = 0; i < block_size; i++) {
                uint64_t u = gen->get_bits(state);
                BlockFrequency_count(&freq, u, 8);
            }
        } else {
            for (size_t i = 0; i < block_size; i++) {
                uint64_t u = gen->get_bits(state);
                BlockFrequency_count(&freq, u, 4);
            }
        }
        BlockFrequency_calc(&freq);
        printf("  Time elapsed: "); print_elapsed_time(time(NULL) - tic);
        printf("\n\n");
    }
    BlockFrequency_free(&freq);
    intf->free(state);
}

/////////////////////////////////////
///// 2D Ising model based test /////
/////////////////////////////////////

typedef struct {
    unsigned int inds[4];
} Neighbours2D;


typedef struct {
    unsigned int L; ///< Lattice size
    unsigned int N; ///< Number of elements, LxL
    int8_t *s; ///< Spins (-1 or +1)
    Neighbours2D *nn; ///< Neigbours
} Ising2DLattice;


static inline int inc_modL(int i, int L)
{
    return (++i) % L;
}

static inline int dec_modL(int i, int L)
{
    i--;
    return (i >= 0) ? i : (L - 1);
}

void Ising2DLattice_init(Ising2DLattice *obj, unsigned int L)
{
    obj->L = L;
    obj->N = L * L;
    obj->s = calloc(obj->N, sizeof(int8_t));
    obj->nn = calloc(obj->N, sizeof(Neighbours2D));
    // Precalculate neighbours indexes
    for (size_t i = 0; i < obj->N; i++) {
        int ix = i % L, iy = i / L;
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


void Ising2DLattice_flip_internal(Ising2DLattice *obj, size_t ind, int8_t s0,
    GeneratorState *gs, const uint64_t p_int)
{
    // Flip spin
    obj->s[ind] = -s0;
    // Go to neighbours
    for (size_t i = 0; i < 4; i++) {
        size_t nn_ind = obj->nn[ind].inds[i];
        uint64_t rnd = gs->gi->get_bits(gs->state);
        if (obj->s[nn_ind] == s0 && rnd <= p_int) {
            Ising2DLattice_flip_internal(obj, nn_ind, s0, gs, p_int);
        }
    }    
}

int Ising2DLattice_calc_energy(const Ising2DLattice *obj)
{
    int energy = 0;
    for (unsigned int i = 0; i < obj->N; i++) {
        energy += (obj->s[i] == obj->s[obj->nn[i].inds[0]]) ? 1 : -1;
        energy += (obj->s[i] == obj->s[obj->nn[i].inds[2]]) ? 1 : -1;
    }
    return energy;
}


void Ising2DLattice_flip(Ising2DLattice *obj, size_t ind, GeneratorState *gs)
{
    const uint64_t p_int = (gs->gi->nbits == 32) ? 2515933592ull : 10806402496730587136ull; // p * 2^nbits
    Ising2DLattice_flip_internal(obj, ind, obj->s[ind], gs, p_int);
}

void Ising2DLattice_free(Ising2DLattice *obj)
{
    free(obj->s); obj->s = NULL;
    free(obj->nn); obj->nn = NULL;    
}


// http://dx.doi.org/10.1103/PhysRevLett.69.3382
// http://dx.doi.org/10.1140/epjst/e2012-01637-8
void battery_ising(GeneratorInfo *gen, const CallerAPI *intf)
{
    Ising2DLattice obj;
    Ising2DLattice_init(&obj, 16);
    size_t sample_len = 15000000, nsamples = 10;
    const double e_ref = 1.4530649029;
    const double cv_ref = 1.4987048885;
    const double jc = log(1 + sqrt(2)) / 2;

    void *state = gen->create(intf);
    GeneratorState gs = {.gi = gen, .state = state, .intf = intf};

    // Warm-up
    for (size_t i = 0; i < sample_len; i++) {
        Ising2DLattice_flip(&obj, gs.gi->get_bits(gs.state) % obj.N, &gs);
    }
    // Sampling
    double *e = calloc(nsamples, sizeof(double));
    double *cv = calloc(nsamples, sizeof(double));
    for (size_t ii = 0; ii < nsamples; ii++) {
        unsigned long long energy_sum = 0, energy_sum2 = 0;
        for (size_t i = 0; i < sample_len; i++) {
            int energy = Ising2DLattice_calc_energy(&obj);
            Ising2DLattice_flip(&obj, gs.gi->get_bits(gs.state) % obj.N, &gs);
            energy_sum += energy;
            energy_sum2 += energy * energy;
        }
        double Emean = (double) energy_sum / sample_len;
        double E2mean = (double) energy_sum2 / sample_len;
        e[ii] = Emean / obj.N;
        cv[ii] = (E2mean - Emean * Emean) / obj.N * jc * jc;
        printf("e = %.8f, cv = %.8f\n", e[ii], cv[ii]);
    }
    // Mean and std
    double e_mean = 0.0, e_std = 0.0, cv_mean = 0.0, cv_std = 0.0;
    for (size_t i = 0; i < nsamples; i++) {
        e_mean += e[i];
        cv_mean += cv[i];
    }
    e_mean /= nsamples;
    cv_mean /= nsamples;
    for (size_t i = 0; i < nsamples; i++) {
        e_std += pow(e[i] - e_mean, 2.0);
        cv_std += pow(cv[i] - cv_mean, 2.0);
    }
    e_std = sqrt(e_std / (nsamples - 1));
    cv_std = sqrt(cv_std / (nsamples - 1));
    printf("e_mean = %g; e_std = %g; z = %g\n", e_mean, e_std,
        (e_mean - e_ref) / (e_std / sqrt(nsamples)));
    printf("cv_mean = %g; cv_std = %g; z = %g\n", cv_mean, cv_std,
        (cv_mean - cv_ref) / (cv_std / sqrt(nsamples)));
    free(e);

    Ising2DLattice_print(&obj);
    Ising2DLattice_free(&obj);
    intf->free(state);
}
