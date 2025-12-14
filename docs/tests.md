# About implemented tests

The next tests are implemented in the SmokeRand core:

1. Monobit frequency test.
2. Frequency test for bytes and 16-bit chunks.
3. Birthday spacings test.
4. Birthday spacings test with decimation.
5. CollisionOver test.
6. Gap test.
7. Gap test on 16-bit words with zero counting (modified `rda16` from gjrand).
8. Matrix rank test.
9. Linear complexity test.
10. Hamming weights frequency tests for non-overlapping blocks and their
    pairwise XORs.
11. Hamming weights tests based on overlapping tuples (similar to DC6 test
    from PractRand 0.94).
12. Simplified `mod3` test from gjrand.
13. SumCollector test.

Systematic failure of even one test means that PRNG is not suitable as a general
purpose PRNG. However different tests have different impact on the PRNG quality:

1. Frequency tests failure: the PRNG is either broken or seriously flawed.
2. Hamming weights based tests: short-term correlations in bits distribution.
   Local deviations from the uniform distribution are possible.
3. Gap test failure: the PRNG output has regularities that may disrupt
   Monte-Carlo simulations (similar to Ising 2D model case).
4. Birthday spacings, CollisionOver and SumCollector test failure: the PRNG
   output shows a regular structure (often similar to lattice) that can break
   generation of identifiers and Monte-Carlo simulations.
5. Matrix rank and linear complexity tests failure: there is a linear dependence
   between bits of the PRNG output sequence. Don't work with separate bits of 
   such PRNG, even `a % n` usage may be risky. However, these flaws are usually
   irrelevant for Monte-Carlo simulations and MT19937 and WELL1024a are often
   used as general purpose PRNGs.

Systematic failures in P.1-4 mean that PRNG is seriously flawed and mustn't
be used for computations. P.5 requires a special consideration for each task
and each generator. Of course, statistical tests are not sufficient for
PRNG quality control: state-of-art quality PRNG should satisfy theoretical
"next bit test", i.e. based on cryptographical primitives such as block or
stream ciphers.

Extra tests:

1. 64-bit birthday paradox test. Requires 8 GiB of RAM and about 30 minutes.
   Allows to detect perfectly uniform 64-bit generators with 64-bit state
   such as SplitMix, 64-bit LCGs with full 64-bit outputs, some modifications
   of PCG.
2. 2-dimensional Ising model test: modifications with Wolff and Metropolis
   algorithms. Rather slow and not very sensitive, but resemble real
   Monte-Carlo computations.
3. Long runs of frequency test for 1-bit, 8-bit and 16-bit chunks. This is an
   adaptive test that will show intermediate results.


# Tests description

The tests supplied with SmokeRand are mostly well-known, described in scientific
literature and were already implemented in other test suits. However some its
modifications are specific to SmokeRand and allow to detect such PRNGs as 
64-bit LCG with prime modulus, 96-bit and 128-bit LCGs with \f$ m = 2^k \f$
and truncation of lower 64 or even 96 bits, "Subtract-with-Borrow + Weyl
Sequence" (SWBW) and incorrectly configured CSPRNGs with 32-bit counters.

Despite relatively small amount of tests SmokeRand can detect flaws in some
PRNG that pass BigCrush or PractRand:

- 64-bit LCG with prime modulus: passes PractRand, detection by BigCrush
  requires special settings (interleaved mode).
- 96-bit and 128-bit LCGs by modulo 2^{k} and truncation of lower 64 bits.
  They pass BigCrush but detected by PractRand. SmokeRand also can detect
  128-bit LCG with truncated lower 96 bits (they pass BigCrush and PractRand).
- SWBW: detected by PractRand but not by BigCrush.
- Uniformly distributed 64-bit generators with 64-bit state such as
  SplitMix, PCG64/64, rrmxmx, DES-CTR, MAGMA-CTR: detected by an extra
  "birthday paradox" battery.
- Additive and subtractive lagged Fibonacci generators with large lags, e.g.
  LFib(19937,9842+): the `gap16` (`rda16`) test is taken from gjrand.
- RC4 obsolete CSPRNG: detected by PractRand but not by BigCrush. In SmokeRand
  extra "freq" battery is required to detect it.

## Monobit frequency test

A classic monobit frequency test that counts frequencies of 0 and 1 in the
input stream of bits and calculates p-value. The next random value is calculated:

\f[
X = \sum_{i=1}^n \left( 1 - 2x_i \right) = n - 2\sum_{i-1}^n x_i
\f]

i.e. 0 is replaced to -1, 1 is replaced to 1. The X random value has a normal
distribution (see central limit theorem) with \f$ E(X) = 0\f$, and
\f$ Var(X) = n \f$. Its absolute value obeys a half-normal distribtion
and p-value is calculated as:

\f[
p = 1 - F(|X|) = 2\left(1 - \Phi\left(\frac{|X|}{\sqrt{n}}\right) \right) = 
\mathop{\textrm{erfc}} \left(\frac{|X|}{\sqrt{2n}}\right)
\f]

Most PRNGs pass it, but it may detect bias in hardware generated bits or some
serious flaws in PRNG implementaton. E.g. multiplicative lagged Fibonacci that
returns all bits won't pass it.

## Blocks frequency test

The test counts frequencies of not overlapping 8-bit and 16-bit blocks of bits.
Chi-squared test is applied to the obtained empirical frequencies to check
if they are uniformly distributed. It may detect some low-grade generators such
as randu, lcg69069, shr3.


## Birthday spacings test

The birthday test generates input values using the next algorithm:

1. Get `ndim` pseudorandom values and take the lower (or higher) `nbits`
   bits from each value.
2. Concatenate them as `x_n | x_{n-1} | ... | x_{1}` ("birthday").

This n-dimensional modification is described by P. L'Ecuyer and R.Simard
(https://doi.org/10.1016/S0378-4754(00)00253-6). Then classic "birthday test"
is applied, number of "birthdays" is adjusted to \f$ \lambda = 4 \f$ value.
The test is repeated several times (`nsamples`), the number of duplicates
from these runs are summed. This sum obeys Poisson distribution.

 Name        | nbits | ndim | `express` | `brief` | `default` | `full` 
-------------|-------|------|-----------|---------|-----------|--------
 bspace64_1d | 64    | 1    | -         | 40      | 100       | 250
 bspace32_1d | 32    | 1    | 1024      | 4096    | 8192      | 8192
 bspace32_2d | 32    | 2    | -         | 5       | 10        | 250
 bspace21_3d | 21    | 3    | -         | 5       | 10        | 250
 bspace16_4d | 16    | 4    | -         | 5       | 10        | 250
 bspace8_8d  | 8     | 8    | -         | 5       | 10        | 250
 bspace8_4d  | 8     | 4    | 256       | -       | -         | -
 bspace4_8d  | 4     | 8    | 128       | -       | -         | -

1-dimensional tests are sensitive to 64-bit LCGs with prime modulus and lagged
Fibonacci generators, 3-dimensional tests detect 32- and 64-bit MWC
(multiply-with-carry) PRNGS, 4D and 8D modifications are very sensitive to
64-bit LCGs with truncated lower 32 bits.

## Birthday spacings test with decimation

This test is a modification of n-dimensional birthday spacings test that throws
out the majority of values. It has `step` property that means the next: return
1 (one) value and throw out `step - 1` values. Then `bspace4_8d` is applied to
the lower and higher bits of values from the obtained decimated sample. It
makes the test very sensitive to LCGs with \f$m = 2^{k}\f$: even 128-bit LCGs
can be detected in less than 1 minute on modern CPU.

## Collision over test

This test is a modification of classic "Monkey test". The algorithm was
suggested by developers of TestU01. Number of collision obeys the Poisson
distribution with the next mathematical expectance:

\f[
\mu = d^t \left(\lambda - 1 + e^{-\lambda} \right)
\f]

where \f$d\f$ is number of states per dimensions, \f$t\f$ is the number
of dimensions and \f$\lambda\f$ is so-called "density":

\f[
\lambda = \frac{n - t + 1}{d^t}
\f]

where \f$n\f$ is the number of points in the sample. The formula are correct
only when \f$ n \ll d^t \f$.


 Name          | nbits | ndim | `brief` | `default` | `full`
---------------|-------|------|---------|-----------|--------
 collover20_2d | 20    | 2    | 3       | 5         | 50
 collover13_3d | 13    | 3    | 3       | 5         | 50
 collover8_5d  | 8     | 5    | 3       | 5         | 50
 collover5_8d  | 5     | 8    | 3       | 5         | 50

## Gap test

A classical gap test described in TAOCP (volume 2). Sensitive to additive and
subtractive lagged Fibonacci and substact-with-borrow generators. Easily 
detects 32-bit PRNGs that exceeded their periods. Even ChaCha12 with 32-bit
counter may fail some modifications of the gap test.

The implementation of gap test in SmokeRand is equipped with "guard function"
that will prevent infinite cycle in the case of constant output from a broken
generator.

## Gap test with 16-bit words

A modification of gap test applied to the stream of non-overlapping 16-bit
words suggested by the gjrand author and known as `rda16`. It simultaneously
analyses 2^17 gaps. Two kinds of gaps are used:

- `[value ... value]`; also with counting fraction of gaps of a given length
  that contain 0 between values.
- `[0 ... value]`; also with counting fraction of gaps of a given length
  that contain value between 0 and value.

This test easily detects 32-bit and 48-bit LCGs with m=2^k, shr3 (xorshift32),
splitmix32, mulberry32, mwc4691. However, it is particularly sensitive to
additive and subtractive lagged Fibonacci generators and SWB (subtract with
borrow) generators.


## Linear complexity test

This specialized test calculates linear complexity of selected bit of PRNG
output. Uses Berlekamp-Massey algorithm. Very sensitive to the generators
based on LFSR and GFSR such as Mersenne twister and xorshift.

 Name            | Bit for 32/64-bit PRNG
-----------------|------------------------
 linearcomp_high | 31/63
 linearcomp_mid  | 15/31
 linearcomp_low  | 0/0


## Matrix rank test

It is a traditional binary matrix rank test implemented for large square binary
matrices; the matrix size must be a multiple of 256 to make SIMD or `uint64_t`
based vectorization easier. They can be applied either to the whole PRNG
output or to its lower 8 bits.

The test generates 64 matrices and calculates number of matrices with
ranks \f$\le(n - 2)\f$, \f$ (n - 1) \f$ and \f$ n \f$ respectively, then
chi-squared test with 2 degrees of freedom is applied.

The matrix rank is sensitive to LFSR and GFSR generators. However, it is less
sensitive than linear complexity tests and won't detect Mersenne twister due to
its huge state. MT19937 will fail it for \f$32768\times 32768\f$ matrices but
due to \f$ O(n^3)\f$ time complexity such run will take several hours in
one-threaded mode. Linear complexity test is much faster in this case.

 Name                 | n      | nbits
----------------------|--------|--------
 matrixrank_4096      | 4096   | 32/64
 matrixrank_4096_low8 | 4096   | 8
 matrixrank_8192      | 8192   | 32/64
 matrixrank_8192_low8 | 8192   | 8

## Hamming weights histogram test

This test divides an input stream into n-bit blocks and calculates Hamming
weights for all blocks. These weights must obey the binomial distribution.
The test is repeated for pairs of blocks: each pair is XORed and Hamming
weights of that XORs are analysed, they also must obey binomial distribution.

This test is designed mainly as basic sanity check for counter-based generator,
it may detect evident flaws in avalanche characteristics. Pairwise XOR may
detect such PRNGs as SplitMix with gamma equal to 1.

The Hamming weights histogram test also catches 32-bit LCGs with modulo
\f$ m={2^32}\f$, additive/subtractive lagged Fibonacci generators with small
lags, ranrot32 with small lags, some small LFSR (shr3, xsh, xorshift128,
lrnd64_255).

## Hamming weights tests based on overlapping tuples.

This test is a modification of `DC6-9x1Bytes-1` test from PractRand by Chris
Doty-Humphrey. Actually it is a family of algorithms that can analyse bytes,
16-bit, 32-bit and 64-bit chunks. The DC6-9x1Bytes-1 modification works with
bytes. SmokeRand `hamming_ot` tests also can process longer 128-bit, 256-bit
and 512-bit chunks that are especially useful for detection of RANROT and
additive/subtractive lagged Fibonacci generators.

 Chunk size, bits | nvalues       | nsamples  | mu    | sigma | lilliefors
------------------|---------------|-----------|-------|-------|------------
 8                | 10^7  / 10M   | 1'000'000 | 0.038 | 1.396 | -
 8                | 10^8  / 100M  |   200'000 | 0.048 | 1.361 | +
 8                | 10^9  / 1000M |    30'000 | 0.059 | 1.332 | +
 8                | 10^10 / 10B   |     3'500 | 0.036 | 1.328 | +
 32               | 10^8  / 100M  |   100'000 | 0.082 | 1.297 | +
 32               | 10^9  / 1000M |    30'000 | 0.065 | 1.295 | +
 32               | 10^10 / 10B   |     1'000 | 0.063 | 1.285 | +
 64               | 10^7  / 10M   | 1'000'000 | 0.023 | 1.295 | -
 64               | 10^8  / 100M  |   500'000 | 0.061 | 1.300 | +
 64               | 10^9  / 1000M |    30'000 | 0.119 | 1.295 | +
 64               | 10^10 / 10B   |     1'000 |-0.039 | 1.222 | +
 128              | 10^8  / 100M  |    10'000 | 0.077 | 1.281 | +
 128              | 10^9  / 1000M |     1'000 | 0.025 | 1.281 | +
 128              | 10^10 / 10B   |     1'000 |-0.040 | 1.257 | +
 256              | 10^8  / 100M  |    10'000 | 0.076 | 1.288 | +
 256              | 10^9  / 1000M |    35'000 | 0.067 | 1.274 | +
 256              | 10^10 / 10B   |     1'000 | 0.079 | 1.263 | +

# Extra tests description

These tests are not included into the `brief`, `default` and `full` batteries
because they are slow, may require a lot of RAM and are usually aimed on some
very specific issues.

## 64-bit birthday paradox test

This test detects uniform generators with 64-bit output and 64-bit state. Such
generators never repeat themselves during the entire period and it can be
easily detected by means of "birthday paradox": number of duplicates in the
sample of n values that have m bits size each obeys Poisson distribution with
the next mathematical expectance:

\f[
\lambda = \frac{n^2}{2\cdot 2^m}
\f]

For a 64-bit generator m=64; to achieve p-value less than 1e-10 for absence of
duplicates we need n around 2^35 that correspond to 256 GiB of data; such
sample is too large for RAM of most personal computers in 2024. SmokeRand uses
the decimation strategy suggested by M.E. O'Neill, the author of PCG generators.
In this case only outputs with lower \f$ e \f$ bits equal to 0 are used, all
other values are thrown out. And \f$ e = 7 \f$ allows to use only 8 GiB of RAM
for \f$\lambda = 4\f$. If no duplicates are found for this \f$e\f$ then another
attempt with \f$e = 9\f$ and \f$\lambda = 16\f$ is made. Then number of
duplicates from both runs are summed and p-value is calculated.

- https://www.pcg-random.org/posts/birthday-test.html

32-bit version of this test consumes less RAM, uses larger e and is much
slower. However, it is made as a rarely used backup variant: ordinary x86 based
workstations in 2024 usually 64-bit and have at least 16 GiB of RAM.

## Extended block frequency test

It is very similar to the blocks frequency test from `brief`, `default` and
`full` battery but it simultaneously processes non-overlapping 8-bit and
16-bit blocks. Distribution uniformity is controlled by Pearson chi2 test
and (for all frequencies) by half-normal distribution test (for every frequency).
Half-normal distribution test (`zmax`) is an approximation of binomial
distribution; it is used together with Bonferroni correction.

This test is not very sensitive. But RC4, obsolete CSPRNG, fails it at 16-bit
chunks at zmax test at 432 GiB of data (PractRand 0.94 fails at 1 TiB). This
test run required about 25 min. Its RC4OK modification doesn't have such
statistical bias and passes this frequency test at least for 1 TiB sample.


## 2D Ising model test

These tests are based on heat capacity and internal energy computation for 2D
16x16 Ising model using Wolff and Metropolis algorithms. Computations based on
Wolff algorithm are approximately equivalent to gap test and give a significant
bias for additive/subtractive lagged Fibonacci generators and subtract with
borrow generators without luxury levels. Metropolis algorithm may detect LCGs
with small state size. Of course, they are slower and less sensitive tha
specialized statistical tests from `express`, `brief`, `default` and `full`
batteries and have mainly historical and educational interest.


# TODO list

- Consider adding SHELL = cmd inside a Windows_NT condition since those commands
  rely on that particular shell, which isn't necessarily what GNU Make will pick
  automatically.
