# Introduction

SmokeRand is a set of tests for pseudorandom number generators. Tested
generators should return either 32-bit or 64-bit unsigned uniformly distributed
unsigned integers. Its set of tests resembles SmallCrush, Crush and BigCrush
from TestU01 but has several important differences:

- Supports 64-bit generators directly, without taking upper/lower parts,
  interleaving etc.
- Direct access to the lowest bits of 32 or 64 bit generator that improves
  sensitivity of tests.
- Minimialistic sets of tests. However, the selected tests are powerful
  enough to detect flaws in a lot of popular PRNGs.
- High speed: `brief` battery runs in less than 1 minute, `default` in less
  than 5 minutes and `full` in less than 1 hour.
- Multithreading support by means of POSIX threads or WinAPI threads.

Despite relatively small amount of tests SmokeRand can detect flaws in some
PRNG that pass BigCrush or PractRand:

- 64-bit LCG with prime modulus: passes PractRand, detection by BigCrush
  requires special settings (interleaved mode).
- 96-bit and 128-bit LCGs by modulo 2^{k} and truncation of lower 64 bits.
  They pass BigCrush but detected by PractRand. SmokeRand also can detect
  128-bit LCG with truncated lower 96 bits (they pass BigCrush and PractRand).
- SWBW: detected by PractRand but not by BigCrush.
- Uniformly distributed 64-bit generators with 64-bit state such as
  SplitMix, PCG64/64, rrmxmx: detected by an extra "birthday paradox" battery.
- Additive and subtractive lagged Fibonacci generators with large lags, e.g.
  LFib(19937,9842+): the `gap16` (`rda16`) test is taken from gjrand.
- RC4 obsolete CSPRNG: detected by PractRand but not by BigCrush. In SmokeRand
  extra "freq" battery is required to detect it.

Existing solutions:

1. TestU01 (https://doi.org/10.1145/3447773). Has a very comprehensive set of
   tests and an excellent documentation. Doesn't support 64-bit generator,
   has issues with analysis of lowest bits.
2. PractRand (https://pracrand.sourceforge.net/). Very limited number of tests
   but they are good ones and detect flaws in a lot of PRNGs. Tests are mostly
   suggested by the PractRand author and is not described in scientific
   publications. Their advantage -- they accumulate information from a sequence
   of 1024-byte blocks and can be applied to arbitrary large samples.
3. Ent (https://www.fourmilab.ch/random/). Only very basic tests, even 64-bit
   LCG will probably pass them.
4. Dieharder (https://webhome.phy.duke.edu/~rgb/General/dieharder.php).
   Resembles DIEHARD, but contains more tests and uses much larger samples.
   Less sensitive than TestU01.
5. gjrand (https://gjrand.sourceforge.net/). Has some unique and sensitive
   tests but documentation is scarce.

Minimal requirements:

- C99 compiler; C99 standard was chosen instead of C89 due to mandatory support
  of 64-bit integers and `inline` keyword.
- GNU make.
- 32-bit CPU.
- 2 GiB of RAM.

Recommended configuration:

- C99 compiler with 128-bit integers support either through `__uint128_t` type
  (GCC, Clang) or `_umul128`/`_addcarry_u64` intrinsics (Microsoft Visual C).
  They are required by some PRNGs such as 128-bit LCGs. In the case of x86-64
  intrinsics for RDTSC, RDRAND and AVX2 are highly recommended.
- Multithreading support: pthreads (POSIX threads) library of WinAPI threads.
- CMake for compilation under MSVC.
- 64-bit CPU; in the case of x86-64 -- support of RDTSC, RDRAND and AVX2
  instructions.
- 16 GiB of RAM, especially for multithreaded mode and/or `birthday` battery.


Implemented tests:

1. Monobit frequency test.
2. Frequency test for bytes and 16-bit chunks.
3. Birthday spacings test.
4. Birthday spacings test with decimation.
5. CollisionOver test.
6. Gap test.
7. Gap test on 16-bit words with zero counting (modified `rda16` from gjrand).
8. Matrix rank test.
9. Linear complexity test.
10. Hamming weights tests based on overlapping tuples (similar to DC6 test
    from PractRand 0.94).
11. Simplified `mod3` test from grand.


Systematic failure of even one test means that PRNG is not suitable as a general
purpose PRNG. However different tests have different impact on the PRNG quality:

1. Frequency tests failure: the PRNG is either broken or seriously flawed.
2. Hamming weights based tests: short-term correlations in bits distribution.
   Local deviations from the uniform distribution are possible.
3. Gap test failure: the PRNG output has regularities that may disrupt
   Monte-Carlo simulations (similar to Ising 2D model case).
4. Birthday spacings and CollisionOver test failure: the PRNG output shows a
   regular structure (often similar to lattice) that can break generation of
   identifiers and Monte-Carlo simulations.
5. Matrix rank and linear complexity tests failure: there is a linear dependence
   between bits of the PRNG output sequence. Don't work with separate bits of 
   such PRNG, even `a % n` usage may be risky. However, these flaws are usually
   irrelevant for Monte-Carlo simulations and MT19937 and WELL1024a are often
   used as general purpose PRNGs.

Systematic failures in P.1-4 mean that PRNG is seriously flawed and mustn't
be used for computations. P.5 requires a special consideration for each task
and each generator. Of course, statistical tests are not sufficient for
PRNG quality control: state-of-art quality PRNG should satisfy theoretical
"next bit test" and be cryptographically secure.

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

# Implemented algorithms

A lot of pseudorandom number generators are supplied with SmokeRand. They can
be divided into several groups:

- Cryptographically secure: chacha, chacha_avx (ChaCha12), isaac64, speck128,
  speck128_avx (Speck128/128).
- Obsolete CSPRNG: rc4.
- Counter-based scramblers based on cryptographical primitives: philox,
  philox32, threefry.
- Lagged Fibonacci: alfib, alfib_mod, mlfib17_5, lfib_par, r1279.
- Linear congruental: cmwc4096, drand48, lcg32prime, lcg64, lcg64prime, lcg96,
  lcg128, lcg128_full, lcg128_u32_full, lcg69069, minstd, mwc1616, mwc64,
  mwc128, randu, ranluxpp, seizgin63.
- Linear congruental with output scrambling: mwc64x, mwc128x, mwc1616x,
  mwc3232x, pcg32, pcg64, pcg64_xsl_rr.
- "Weyl sequence" (LCG with a=1) with output scrambling: mulberry32, rrmxmx,
  splitmix, splitmix32, sqxor, sqxor32, wyrand.
- "Weyl sequence" injected into reversible nonlinear transformation: sfc8,
  sfc16, sfc32, sfc64
- "Weyl sequence" injected into irreversible nonlinear transformation: cwg64,
  stormdrop, msws.
- Subtract with borrow: swb, swblux, swbw.
- LSFR without scrambling: shr3, xsh, xorshift128, lfsr113, lfsr258, well1024a.
- LSFR with scrambling: xorshift128p, xoroshiro128p, xoroshiro128pp,
  xoroshiro1024st, xoroshiro1024stst, xorwow.
- GSFR: mt19937, tinymt32, tinymt64.
- Combined generators: kiss93, kiss99, kiss64, superduper73, superduper64,
  superduper64_u32.
- Other: coveyou64, mrg32k3a, romutrio.


 Algorithm         | Description
-------------------|-------------------------------------------------------------------------
 alfib             | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod         | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 chacha            | ChaCha12 CSPRNG: Cross-platform implementation
 coveyou64         | COVEYOU
 isaac64           | ISAAC64 64-bit CSPRNG
 kiss93            | KISS93 combined generator by G.Marsaglia
 kiss99            | KISS99 combined generator by G.Marsaglia
 kiss64            | 64-bit version of KISS
 lcg32prime        |
 lcg64             | \f$ LCG(2^{64},6906969069,1) \f$ that returns upper 32 bits
 lcg64prime        | \f$ LCG(2^{64}-59,a,0)\f$ that returns all 64 bits
 lcg96             | \f$ LCG(2^{96},a,1) \f$ that returns upper 32 bits
 lcg128            | \f$ LCG(2^{128},18000690696906969069,1) \f$, returns upper 32/64 bits
 lcg69069          | \f$ LCG(2^{32},69069,1)\f$, returns whole 32 bits
 lfsr113           |
 lfsr258           |
 minstd            | \f$ LCG(2^{31} - 1, 16807, 0)\f$ "minimial standard" obsolete generator.
 mlfib17_5         | \f$ LFib(x,2^{64},17,5) \f$
 mrg32k3a          | MRG32k3a
 mt19937           | Mersenne twister.
 mulberry32        | Mulberry32 generator.
 mwc64             | Multiply-with-carry generator with 64-bit state
 mwc64x            | MWC64X: modification of MWC64 that returns XOR of x and c
 mwc128            | Multiply-with-carry generator with 64-bit state
 mwc128x           | MWC128X: similar to MWC64X but x and c are 64-bit
 pcg32             | Permuted Congruental Generator (32-bit version, 64-bit state)
 pcg64             | Permuted Congruental Generator (64-bit version, 64-bit state)
 philox            | Philox4x64x10 counter-based generator.
 philox32          | Philox4x32x10 counter-based generator.
 randu             | \f$ LCG(2^{32},65539,1) \f$, returns whole 32 bits
 r1279             | \f$ LFib(XOR, 2^{32}, 1279, 1063) \f$ generator.
 rc4               | RC4 obsolete CSPRNG (doesn't pass PractRand)
 rrmxmx            | Modified SplitMix PRNG with improved output function
 seigzin63         | \f$ LCG(2^{63}-25,a,0) \f$
 sfc32             | "Small Fast Chaotic 64-bit" PRNG by Chris Doty-Humphrey
 sfc64             | "Small Fast Chaotic 64-bit" PRNG by Chris Doty-Humphrey
 speck128          | Speck128/128 CSPRNG.
 speck128_avx      | Modification of `speck128` for AVX2.
 splitmix32        | 32-bit modification of SplitMix
 sqxor             |
 sqxor32           |
 superduper73      |
 superduper64      |
 superduper64_u32  |
 shr3              | xorshift32 generator by G.Marsaglia
 swb               | 32-bit SWB (Subtract with borrow) generator by G.Marsaglia
 swblux            | Modification of SWB with 'luxury levels' similar to RANLUX
 swbw              | Modification of SWB combined with 'discrete Weyl sequence'
 tinymt32          | "Tiny Mersenne Twister": 32-bit version
 tinymt64          | "Tiny Mersenne Twister": 64-bit version
 threefry          | Threefry4x64x20 counter-based generator
 well1024a         | WELL1024a: Well equidistributed long-period linear
 xorshift128       | xorshift128 LFSR generator by G.Marsaglia
 xorshift128p      | xorshift128+, based on xorshift128, uses an output scrambler.
 xoroshiro128p     | xoroshiro128+
 xoroshiro128pp    | xoroshiro128++
 xoroshiro1024st   | xoroshiro1024*
 xoroshiro1024st   | xoroshiro1024**
 xorwow            | xorwow
 xsh               | xorshift64 generator by G.Marsaglia


# Batteries

Four batteries are implemented in SmokeRand:

- `brief` - a fast battery that includes a reduced set of tests, e.g.
  matrix rank tests and some tests for higher bits of generators
  are excluded.
- `default` - a more comprehensive but slower battery with extra tests
  for higher bits of generators and matrix rank tests. Other tests use
  larger samples that make them more sensitive.
- `full` - similar do default but uses larger samples. 
- `dos16` - simplified battery that consists of 5 tests and can be
  compiled for 16-bit platforms with 64KiB segments of data and code.
  Less powerful than `brief` but may be more sensitive than diehard
  or even dieharder.


 Battery | Number of tests | Bytes (32-bit PRNG) | Bytes (64-bit PRNG)
---------|-----------------|---------------------|---------------------
 brief   | 22              | 2^35                | 2^36
 default | 40              | 2^37                | 2^38
 full    | 44              | 2^40                | 2^41
 dos16   | 5               | 2^31                | 2^32


# How to test a pseudorandom number generator

Two ways of PRNG testing are implemented in SmokeRand:

1. Through stdin/stdio pipes. Simple but doesn't support multithreading.
2. Load PRNG from a shared library. More complex but allows multithreading.



# Tests description

The tests supplied with SmokeRand are mostly well-known, described in scientific
literature and were already implemented in other test suits. However some its
modifications are specific to SmokeRand and allow to detect such PRNGs as 
64-bit LCG with prime modulus, 96-bit and 128-bit LCGs with \f$ m = 2^k \f$
and truncation of lower 64 or even 96 bits, "Subtract-with-Borrow + Weyl
Sequence" (SWBW) and incorrectly configured CSPRNGs with 32-bit counters.

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
\erfc \left(\frac{|X|}{\sqrt{2n}}\right)
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

 Name        | nbits | ndim | `brief` | `default` | `full` 
-------------|-------|------|---------|-----------|--------
 bspace64_1d | 64    | 1    | 40      | 100       | 250
 bspace32_1d | 32    | 1    | 4096    | 8192      | 8192
 bspace32_2d | 32    | 2    | 5       | 10        | 250
 bspace21_3d | 21    | 3    | 5       | 10        | 250
 bspace16_4d | 16    | 4    | 5       | 10        | 250
 bspace8_8d  | 8     | 8    | 5       | 10        | 250

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

## Hamming weights tests based on overlapping tuples.

This test is a modification of `DC6-9x1Bytes-1` test from PractRand by Chris
Doty-Humphrey. Actually it is a family of algorithms that can analyse bytes,
16-bit, 32-bit and 64-bit chunks. The DC6-9x1Bytes-1 modification works with
bytes. SmokeRand `hamming_ot` tests also can process longer 128-bit, 256-bit
and 512-bit chunks that are especially useful for detection of RANROT and
additive/subtractive lagged Fibonacci generators.

# Extra tests description

These tests are not included into the `brief`, `default` and `full` batteries
because they are slow, may require a lot of RAM and are usually aimed on some
very specific issues.

## 64-bit birthday paradox test

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




# Tests results

 Algoritrhm        | Output | brief | default | full | cpb  | dos16 | bday64 | TestU01 | PractRand 
-------------------|--------|-------|---------|------|------|-------|--------|---------|-----------
 aesni128          | u64    | +     | +       | +    | 0.89 | +     | +      |         | >= 1 TiB
 alfib             | u64    | 5     | 6       | 8    | 0.23 | 2     | +      | Small   | 128 MiB
 alfib_lux         | u32    | 1     | 1       | 1    | 6.1  | +     | N/A    |         | 4 GiB
 alfib_mod         | u32    | +     | +       | +    | 0.50 | +     | N/A    | +       | 1 TiB
 ara32             | u32    | +     | 1       | 1    | 0.96 | +     | N/A    | +       | 512 MiB
 chacha            | u32    | +     | +       | +    | 2.0  | +     | N/A    | +       |
 chacha_avx        | u32    | +     | +       | +    | 0.7  | +     | N/A    | +       |
 chacha_ctr32      | u32    | +     | +       | 1    | 2.0  | +     | N/A    |         | 256 GiB
 cmwc4096          | u32    | +     | +       | +    | 0.43 | +     | N/A    | +       | >= 32 TiB
 coveyou64         | u32    | 3     | 4       | 4    | 0.62 | 1     | N/A    | Small   | 256 KiB
 cwg64             | u64    | +     | +       | +    | 0.30 | +     | +      |         | >= 1 TiB
 drand48           | u32    | 12    | 20      | 22/23| 0.72 | 1     | N/A    | -       | 1 MiB
 isaac64           | u64    | +     | +       | +    | 0.75 | +     | +      | +       | >= 1 TiB
 flea32x1          | u32    | +     | 1       | 1    | 0.48 | +     | N/A    | +       | 4 MiB
 hc256             | u32    | +     | +       | +    | 1.1  | +     | N/A    |         | >= 1 TiB
 kiss93            | u32    | 1     | 3       | 5    | 0.82 | 1     | N/A    | Small   | 1 MiB
 kiss99            | u32    | +     | +       | +    | 1.0  | +     | N/A    | +       | >= 8 TiB
 kiss64            | u64    | +     | +       | +    | 0.53 | +     | +      | +       | >= 4 TiB
 lcg32prime        | u32    | 13    | 24      | 25/26| 2.2  | 0/1   | N/A    |         | 512 MiB
 lcg64             | u32    | 6     | 8       | 11   | 0.40 | 1     | N/A    | Small   | 16 MiB
 lcg64prime        | u64    | 1     | 1       | 1    | 1.5  | +     | -      | +-      | >= 32 TiB
 lcg96             | u32    | 1     | 1       | 1    | 0.78 | +     | N/A    | +       | 32 GiB
 lcg128            | u64    | 1     | 1       | 1    | 0.35 | +     | +      | +       | 64 GiB
 lcg128_full       | u64    | 1     | 1       | 1    | 0.42 | +     | +      | +       | 64 GiB
 lcg128_u32_full   | u32    | +     | 1       | 1    | 0.75 | +     | N/A    | +       | >= 32 TiB
 lcg69069          | u32    | 19    | 37/38   | 42   | 0.38 | 4     | N/A    | -       | 2 KiB
 lfib_par[31+]     | u32    | 5/6   | 6/7     | 10   | 0.70 | 1     | N/A    | -       | 32 MiB
 lfib_par[55+]     | u32    | 4     | 5       | 7    | 0.51 | 1     | N/A    | -       | 2 GiB
 lfib_par[55-]     | u32    | 4     | 5       | 7    | 0.51 | 1     | N/A    | -       | 2 GiB
 lfib_par[127+]    | u32    | 4     | 4       | 5    | 0.48 | 1     | N/A    | -/Small | 512 MiB
 lfib_par[127-]    | u32    | 4     | 4       | 5    | 0.48 | 1     | N/A    | -/Small | 512 MiB
 lfib_par[258+]    | u32    | 4     | 4       | 5    | 0.44 | 1     | N/A    | Small   | 8 GiB
 lfib_par[258-]    | u32    | 4     | 4       | 5    | 0.46 | 1     | N/A    | Small   | 8 GiB
 lfib_par[378+]    | u32    | 4     | 4       | 5    | 0.46 | 1     | N/A    | Small   | 32 GiB
 lfib_par[378-]    | u32    | 4     | 4       | 5    | 0.45 | 1     | N/A    | Small   | 32 GiB
 lfib_par[607+]    | u32    | 4     | 4       | 5    | 0.41 | 1     | N/A    | Small   | 256 GiB
 lfib_par[607-]    | u32    | 4     | 4       | 5    | 0.40 | 1     | N/A    | Small   | 256 GiB
 lfib_par[1279+]   | u32    | 3/4   | 3/4     | 4/5  | 0.40 | 1     | N/A    | Crush   | 1 TiB
 lfib_par[1279-]   | u32    | 3/4   | 3/4     | 4/5  | 0.40 | 1     | N/A    | Crush   | 1 TiB
 lfib_par[2281+]   | u32    | 3     | 3       | 4    | 0.38 | +     | N/A    | +       | 8 TiB
 lfib_par[2281-]   | u32    | 3     | 3       | 4    | 0.38 | 0/1   | N/A    | +       | 8 TiB
 lfib_par[3217+]   | u32    | 1     | 1       | 1/2  | 0.39 | +     | N/A    | +       |
 lfib_par[3217-]   | u32    | 1     | 1       | 2/4  | 0.39 | +     | N/A    | +       |
 lfib_par[9689+]   | u32    | 1     | 1       | 1    | 0.47 | +     | N/A    | +       |
 lfib_par[9689-]   | u32    | 1     | 1       | 1    | 0.47 | +     | N/A    | +       |
 lfib_par[19937+]  | u32    | +     | 1       | 1    | 0.46 | +     | N/A    | +       |
 lfib_par[19937-]  | u32    | +     | 1       | 1    | 0.48 | +     | N/A    | +       |
 lfib_par[44497+]  | u32    | +     | 1       | 1    | 0.49 | +     | N/A    | +       |
 lfib_par[44497-]  | u32    | +     | 1       | 1    | 0.49 | +     | N/A    | +       |
 lfib_par[110503+] | u32    | +     | +       | +    | 0.52 | +     | N/A    | +       |
 lfib_par[110503-] | u32    | +     | +       | +    | 0.50 | +     | N/A    | +       |
 lfib4             | u32    | 1     | 3       | 4    | 0.37 | 1     | N/A    |         | 32 MiB
 lfib4_u64         | u32    | +     | +       | +    | 0.34 | +     | N/A    |         | >= 2 TiB
 lfsr113           | u32    | 3     | 5       | 7    | 1.1  | 2     | N/A    |         | 32 KiB 
 lfsr258           | u64    | 3     | 5       | 7    | 0.75 | 2     | +      |         | 1 MiB
 minstd            | u32    | 20    | 38      | 42   | 2.4  | 4     | N/A    | -       | 1 KiB
 mixmax_low32      | u32    | +     | +       | +    | 1.7  | +     | N/A    |         | >= 2 TiB
 mlfib17_5         | u32    | +     | +       | +    | 0.48 | +     | N/A    | +       | >= 32 TiB
 mt19937           | u32    | 3     | 3       | 3    | 0.50 | +     | N/A    | Small   | 128 GiB
 mrg32k3a          | u32    | +     | +       | +    | 2.5  | +     | N/A    |         | >= 4 TiB
 msws              | u32    | +     | +       | +    | 0.72 | +     | N/A    | +       | >= 2 TiB
 mulberry32        | u32    | 1     | 2       | 4    | 0.51 | +     | N/A    | Small   | 512 MiB
 mwc32x            | u32    | 2     | 2       | 7    | 1.5  | +     | N/A    | Small   | 128 MiB
 mwc64             | u32    | 1     | 2       | 4    | 0.37 | +     | N/A    | Small   | 1 TiB
 mwc64x            | u32    | +     | +       | +    | 0.53 | +     | N/A    | +       | >= 16 TiB
 mwc128            | u64    | +     | +       | +    | 0.30 | +     | +      | +       | >= 16 TiB
 mwc128x           | u64    | +     | +       | +    | 0.30 | +     | +      | +       | >= 8 TiB
 mwc1616           | u32    | 10/11 | 12/18   | 19   | 0.48 | +     | N/A    |         | 16 MiB
 mwc1616x          | u32    | +     | +       | +    | 1.2  | +     | N/A    | +       | >= 32 TiB(?)
 mwc3232x          | u64    | +     | +       | +    | 0.30 | +     | +      |         | >= 32 TiB
 pcg32             | u32    | +     | +       | +    | 0.44 | +     | N/A    | +       | >= 2 TiB
 pcg64             | u64    | +     | +       | +    | 0.28 | +     | -      | +       | >= 2 TiB
 pcg64_xsl_rr      | u64    | +     | +       | +    | 0.43 | +     | +      |         | >= 32 TiB
 philox            | u64    | +     | +       | +    | 1.7  | +     | +      | +       | >= 2 TiB
 philox32          | u32    | +     | +       | +    | 2.7  | +     | N/A    | +       | >= 2 TiB
 ran               | u64    | +     | +       | +    | 0.43 | +     | +      |         | >= 1 TiB
 ranq1             | u64    | 1     | 3       | 6    | 0.32 | 1     | -      |         | 512 KiB
 ranq2             | u64    | +     | 1       | 2    | 0.33 | +     | +      |         | 2 MiB
 randu             | u32    | 21    | 39      | 43   | 0.41 | 4     | N/A    | -       | 1 KiB
 ranlux++          | u64    | +     | +       | +    | 2.4  | +     | +      | +       | >= 1 TiB
 ranrot32[7/3]     | u32    | 1/2   | 4/5     | 5    | 0.58 | +     | N/A    | Small   | 128 MiB
 ranrot32[17/9]    | u32    | 1     | 2       | 3    | 0.68 | +     | N/A    | +       | 1 GiB
 ranrot32[57/13]   | u32    | +     | +       | 1    | 0.74 | +     | N/A    | +       | 8 GiB
 ranshi            | u64    | 1     | 6       | 7    | 0.43 | +     |        |         | 32 KiB
 ranshi_upper32    | u32    | +     | +       | +    | 0.86 | +     | N/A    |         | >= 2 TiB
 ranshi_lower32    | u32    | +     | +       | +    | 0.86 | +     | N/A    |         | >= 1 TiB
 ranval            | u32    | +     | +       | +    | 0.31 | +     | N/A    |         | >= 4 TiB
 r1279             | u32    | 5     | 7       | 10   | 0.47 | 2     | N/A    |         | 64 MiB
 ranlux[0]         | u32    | 6     | 9       | 12   | 6.8  | 1     | N/A    |         | 4 MiB
 ranlux[1]         | u32    | +     | 2       | 3    | 13   | +     | N/A    |         | 4 GiB
 ranlux[2]         | u32    | +     | +       | +    | 27   | +     | N/A    |         | >= 128 GiB
 rc4               | u32    | +     | +       | +    | 6.0  | +     | N/A    | +       | 512 GiB
 rc4ok             | u32    | +     | +       | +    | 6.6  | +     | N/A    |         |
 romutrio          | u64    | +     | +       | +    | 0.15 | +     | +      |         | >= 1 TiB
 rrmxmx            | u64    | +     | +       | +    | 0.14 | +     | -      |         | >= 2 TiB
 sapparot          | u32    | 1     | 3       | 4    | 0.70 | +     | N/A    |         | 8 MiB
 sapparot2         | u32    | +     | +       | +    | 0.42 | +     | N/A    |         | 2 TiB
 sezgin63          | u32    | +     | 1       | 3    | 3.0  | +     | N/A    |         | >= 16 TiB
 sfc8              | u32    | 3     | 7       | 14   | 1.9  | +     | N/A    |         | 128 MiB
 sfc16             | u32    | +     | +       | +    | 0.93 | +     | N/A    |         | 128 GiB(stdin32)*
 sfc32             | u32    | +     | +       | +    | 0.24 | +     | N/A    |         | >= 4 TiB
 sfc64             | u64    | +     | +       | +    | 0.10 | +     | +      | +       | >= 1 TiB
 speck128          | u64    | +     | +       | +    | 3.1  | +     | +      |         | >= 2 TiB
 speck128_avx      | u64    | +     | +       | +    | 0.65 | +     | +      |         | >= 2 TiB
 splitmix          | u64    | +     | +       | +    | 0.19 | +     | -      | +       | >= 2 TiB
 splitmix32        | u32    | 2     | 3       | 4/5  | 0.25 | +     | N/A    | Small   | 1 GiB
 sqxor             | u64    | +     | +       | +    | 0.13 | +     | +      | +       | >= 16 TiB
 sqxor32           | u32    | 1     | 2       | 4    | 0.20 | +     | N/A    | Small   | 16 GiB
 stormdrop         | u32    | +     | +       | 1    | 1.2  | +     | N/A    |         | >= 1 TiB
 superduper73      | u32    | 9     | 15      | 18   | 0.64 | 1     | N/A    |         | 32 KiB
 superduper64      | u64    | 1     | 3       | 5    | 0.35 | 1     | +      |         | 512 KiB
 superduper64_u32  | u32    | +     | +       | +    | 0.70 | +     | N/A    |         | >= 2 TiB
 shr3              | u32    | 15    | 32      | 36   | 0.76 | 2     | N/A    | -       | 32 KiB
 swb               | u32    | 5     | 6       | 8    | 2.7  | 1     | N/A    | Small   | 128 MiB
 swblux[luxury=1]  | u32    | +     | +       | 0/1  | 6.3  | +     | N/A    | Crush   | 4 TiB
 swbw              | u32    | 1     | 1       | 1    | 2.8  | +     | N/A    | +       | 4 GiB
 tinymt32          | u32    | 2     | 4       | 6    | 1.5  | 1     | N/A    | +       | 4 GiB
 tinymt64          | u64    | 1     | 2       | 4    | 2.7  | 1     | +      |         | 32 GiB
 threefry          | u64    | +     | +       | +    | 1.0  | +     | +      | +       | >= 1 TiB
 well1024a         | u32    | 3     | 5       | 7    | 1.0  | 2     | N/A    | Small   | 64 MiB
 wyrand            | u64    | +     | +       | +    | 0.08 | +     | +      |         | >= 1 TiB
 xorshift128       | u32    | 4     | 6/7     | 8    | 0.41 | 2     | N/A    | -       | 128 KiB
 xorshift128p      | u64    | 1     | 2       | 3    | 0.21 | 1     | +      |         | 32 GiB
 xoroshiro128p     | u64    | 1     | 2       | 3    | 0.16 | 1     | +      |         | 16 MiB
 xoroshiro128pp    | u64    | +     | +       | +    | 0.20 | +     | +      |         | >= 2 TiB
 xoroshiro1024st   | u64    | 1     | 1       | 2    | 0.33 | 1     | +      |         | 128 GiB
 xoroshiro1024stst | u64    | +     | +       | +    | 0.33 | +     | +      | +       | >= 1 TiB
 xorwow            | u32    | 3     | 7       | 9    | 0.52 | 1     | N/A    | Small   | 128 KiB
 xsh               | u64    | 8     | 13      | 17   | 0.43 | 2     | -      | -       | 32 KiB

Note about `mt19937` and `philox`: speed significantly depends on gcc optimization settings:
e.g. changing `-O2` to `-O3` speeds up `mt19937` but slows down `philox`; gcc 10.3.0 (tdm64-1).

Note about `sfc16`: if its output is processed as `stdin16` by PractRand 0.94
then it passes it at >= 1 TiB. But if it is tested as 32-bit PRNG in `stdin32`
mode then it fails PractRand 0.94 at 256 GiB:

    rng=RNG_stdin32, seed=unknown
    length= 64 gigabytes (2^36 bytes), time= 183 seconds
      no anomalies in 263 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 128 gigabytes (2^37 bytes), time= 362 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]Gap-16:A                 R=  +6.3  p =  1.7e-4   unusual
      [Low8/32]Gap-16:B                 R=  +8.9  p =  1.7e-7   very suspicious
      ...and 271 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 256 gigabytes (2^38 bytes), time= 699 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]Gap-16:A                 R= +11.1  p =  1.2e-7   very suspicious
      [Low8/32]Gap-16:B                 R= +22.6  p =  3.2e-19    FAIL !
      ...and 282 test result(s) without anomalies

About `lcg64prime`: it passes BigCrush if upper 32 bits are returned, but
fails it in interleaved mode (fails test N15 `BirthdaySpacings, t = 4`).

About `mwc1616x`: it passes until 32 TiB but it will probably fail at larger
samples. Extra testing is required!

    rng=RNG_stdin32, seed=unknown
    length= 8 terabytes (2^43 bytes), time= 28773 seconds
      no anomalies in 331 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 56248 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low1/32]BCFN(2+6,13-0,T)         R=  -9.1  p =1-7.8e-5   unusual
      ...and 338 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 120904 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +10.8  p =  2.6e-5   mildly suspicious
      FPF-14+6/16:all                   R=  +4.8  p =  5.4e-4   unusual
      ...and 345 test result(s) without anomalies

Sensitivity of dieharder is lower than TestU01 and PractRand:

- Failed dieharder: lcg69069, lcg32prime, minstd, randu, shr3, xsh, drand48, lfib31
- Passed dieharder: lcg64

# Versions history

16.12.2024: SmokeRand 0.15.

- RC4OK generator was added.
- Fine tuning of some `hamming_ot_low` tests in `full` battery: samples sizes
  are doubled.
- Added results of several tests.
- `-O3` key in `Makefile.gnu`.

12.12.2024: SmokeRand 0.14.

- AES-128 based PRNG added (only for x86-64 processors with AESNI support).
- Bugfix in linearcomp test (memory leak).
- An improved report in `sr_tiny.c` --- compact version of `dos16` battery for
  16-bit computers.

09.12.2024: SmokeRand 0.13.

- Bugfix in `nbit_words_freq_test`: more accurate sample size. Also input
  arguments were packed into structure.
- `dos16` battery: samples sizes are changed to fit into ~64 MiB of data
  (16 MiB of data for hypothetical sample reusage between tests). Preparation
  to finishing its compact 16-bit version.

09.12.2024: SmokeRand 0.12. Bugfix in `nbit_words_freq_test`: in the case of
16-bit words the words were overlapping (shift was fixed). SumCollector
test added. Several new generators.

02.12.2024: SmokeRand 0.11, ranrot32 PRNG was added. Also a new modification
of Hamming weights based test was added to `default` and `full` battery (uses
256-bit words, allows to find longer-range correlations and detect lagged
Fibonacci generators and RANROT).

01.12.2024: SmokeRand 0.1, an initial version. Requires some testing, extension
of documentation, completion of `dos16` battery implementation for 16-bit
computers. Tests lists for batteries are frozen before 1.0 release.


# Notes about TMFn test from PractRand 0.94

It is used in PractRand and catches LCGs with power of 2 modulo, i.e.
\f$ m = 2^{32}\f$, \f$ m = 2^{64}\f$, \f$ m = 2^{128}\f$. It is sensitive
enough to detect 128-bit LCG with truncated lower 64 bits but doesn't detect
96-bit PRNG.

 Truncated bits | Fails at
----------------|------------------------------------
 32             | 32 MiB
 40             | 256 MiB
 48             | 2 GiB
 50             | 2 GiB
 52             | 4 GiB
 54             | 8 GiB
 56             | 8 GiB
 58             | 16 GiB
 60             | 16 GiB
 64             | 64 GiB
 65 = 64 + 1    | 64 GiB; 64 GiB; 64 GiB
 66 = 64 + 2    | 128 GiB; 128 GiB
 67 = 64 + 3    | 128 GiB
 68 = 64 + 4    | 128 GiB; 128 GiB; 128 GiB
 69 = 64 + 5    | 256 GiB; 256 GiB
 70 = 64 + 6    | 256 GiB; 256 GiB; 256 GiB
 71 = 64 + 7    | 256 GiB
 72 = 64 + 8    | 512 GiB; 512 GiB; 512 GiB; 512 GiB
 73 = 54 + 10   | 512 GiB; 512 GiB
 74 = 54 + 12   | 1 TiB
 75 = 54 + 14   | NOTE: doesn't fail at 4 TiB!

Notes from PractRand 0.94 author:

https://sourceforge.net/p/pracrand/discussion/366935/thread/eb8d7f3e06/

TMFn is tripple-mirror-frequency, n-level, I think. I'm not thrilled with it,
but LCGs were doing entirely too well on my other tests, so I decided to use
something that could do a reasonable job of spotting power-of-2-modulus LCGs
without wasting too many cycles. It looks for really obvious patterns in three
symmetrically spaced small sets of bits (that is, it draws a set of N bits,
and another set of N bits from M bits later in the stream, and then a third set
from 2xM bits later in the stream, concatonates those sets of bits, and does
a chi-squared test directly on the distribution of those values... where N is
like 3 and M varies, but IIRC is generally rather large and probably a power
of 2), at a variety of different spacings (powers of 2 IIRC... large ones I
think). It skips most of the data IIRC, on a very simple hardwired scheme
(I think it only looks at the first 64 bits out of each kilobyte or something
like that) to avoid wasting too many cycles on this. Hopefully someday I'll get
rid of this when I have a test I like better that does a reasonable job on p2m
LCGs. 2+2 is... the log2 of the number of kilobytes of spacing it's looking at
I think? Mostly it catches LCGs, though some other stuff shows up once
in a while.

Birthday spacings test with decimation is much more sensitive. If there is no
decimation it consumes 4096 * 4 * 8 = 131072 bytes = 2^17 bytes for generator
with 32-bit output. Samples sizes in the case of decimation:

 Decimation            | Sample size* | Detects
-----------------------|--------------|-----------------------------------------------
 \f$2^{6}\f$  (64)     | 8 MiB        | 64-bit LCG with truncation of lower 32 bits.
 \f$2^{12}\f$ (4096)   | 512 MiB      | 128-bit LCG with truncation of lower 64 bits.
 \f$2^{18}\f$ (262144) | 32 GiB       | 128-bit LCG with truncation of lower 96 bits.
 \f$2^{24}\f$          | 2 TiB        | Upper 8 bits from 128-bit LCG.

* For 32-bit output; for 64-bit output it is doubled

It means that `bspace4_8d_dec` is about 100-1000 times more sensitive than TMFn
test from PractRand 0.94 at least for 128-bit LCGs. It is especially true if at
least lower 64 bits are truncated.

