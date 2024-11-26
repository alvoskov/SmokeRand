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

- 64-bit LCG with prime modulus: passes PractRand, some blips on BigCrush.
- 96-bit and 128-bit LCGs by modulo 2^{k} and truncation of lower 64 bits.
  They pass BigCrush but detected by PractRand.
- SWBW: detected by PractRand but not by BigCrush.
- Uniformly distributed 64-bit generators with 64-bit state such as
  SplitMix, PCG64/64, rrmxmx: detected by an extra "birthday paradox" battery.
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
7. Matrix rank test.
8. Linear complexity test.
9. Hamming weights based DC6 test from PractRand.


Systematic failure of even one test means that PRNG is not suitable as a general
purpose PRNG. However different tests have different impact on the PRNG quality:

1. Frequency tests failure: the PRNG is either broken or seriously flawed.
2. Hamming weights based DC6 tests: short-term correlations in bits distribution.
   Local deviations from uniform distribution are possible.
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
 brief   | 20              | 2^35                | 2^36
 default | 34              | 2^37                | 2^38
 full    | 37              | 2^40                | 2^41
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

## Hamming weights based "DC6" test

This test is a modification of `DC6-9x1Bytes-1` test from PractRand by Chris
Doty-Humphrey. Actually it is a family of algorithms that can analyse bytes,
16-bit, 32-bit and 64-bit chunks. The DC6-9x1Bytes-1 modification works with
bytes.

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
test run required about 25 min.


## 2D Ising model test




# Tests results

 Algoritrhm        | Output | brief | default | full | cpb  | dos16 | bday64 | TestU01 | PractRand 
-------------------|--------|-------|---------|------|------|-------|--------|---------|-----------
 alfib             | u64    | 4     | 4       | 7    | 0.23 | 2     | +      | Small   | 128 GiB
 alfib_mod         | u32    | +     | +       | +    | 0.50 | +     | N/A    | +       | 1 TiB
 chacha            | u32    | +     | +       | +    | 2.0  | +     | N/A    | +       |
 chacha_avx        | u32    | +     | +       | +    | 0.7  | +     | N/A    | +       |
 chacha_ctr32      | u32    | +     | +       | 1    | 2.0  | +     | N/A    |         | 256 GiB
 cmwc4096          | u32    | +     | +       | +    | 0.43 | +     | N/A    | +       | >= 32 TiB
 coveyou64         | u32    | 3     | 4       | 4    | 0.62 | 1     | N/A    | Small   | 256 KiB
 cwg64             | u64    | +     | +       | +    | 0.38 | +     | +      |         | >= 1 TiB
 drand48           | u32    | 12    | 19      | 21   | 0.72 | 1     | N/A    | -       | 1 MiB
 isaac64           | u64    | +     | +       | +    | 0.75 | +     | +      | +       | >= 1 TiB
 kiss93            | u32    | 1     | 3       | 5    | 0.82 | 1     | N/A    | Small   | 1 MiB
 kiss99            | u32    | +     | +       | +    | 1.0  | +     | N/A    | +       | >= 8 TiB
 kiss64            | u64    | +     | +       | +    | 0.53 | +     | +      | +       | >= 4 TiB
 lcg32prime        | u32    | 12    | 23      | 24   | 2.2  | 1     | N/A    |         | 512 MiB
 lcg64             | u32    | 6     | 8       | 11   | 0.40 | 1     | N/A    | Small   | 16 MiB
 lcg64prime        | u64    | 1     | 1       | 1    | 1.5  | +     | -      | +-      | >= 32 TiB
 lcg96             | u32    | 1     | 1       | 1    | 0.78 | 1     | N/A    | +       | 32 GiB
 lcg128            | u64    | 1     | 1       | 1    | 0.35 | 1     | +      | +       | 64 GiB
 lcg128_full       | u64    | 1     | 1       | 1    | 0.42 | 1     | +      | +       | 64 GiB
 lcg128_u32_full   | u32    | +     | 1       | 1    | 0.75 | +     | N/A    | +       | >= 32 TiB
 lcg69069          | u32    | 17    | 33      | 36   | 0.38 | 4     | N/A    | -       | 2 KiB
 lfsr113           | u32    | 3     | 5       | 7    | 1.1  | 2     | N/A    |         | 32 KiB 
 lfsr258           | u64    | 3     | 5       | 7    | 0.75 | 2     | +      |         | 1 MiB
 minstd            | u32    | 17    | 33      | 36   | 2.4  | 4     | N/A    | -       | 1 KiB
 mlfib17_5         | u32    | +     | +       | +    | 0.48 | +     | N/A    | +       | >= 4 TiB
 mt19937           | u32    | 3     | 3       | 3    | 0.91 | 2     | N/A    | Small   | 128 GiB
 mrg32k3a          | u32    | +     | +       | +    | 2.5  | +     | N/A    |         | >= 4 TiB
 msws              | u32    | +     | +       | +    | 0.72 | +     | N/A    | +       | >= 2 TiB
 mulberry32        | u32    | 1     | 2       | 3    | 0.51 | +     | N/A    |         | 512 MiB
 mwc32x            | u32    | 1     | 1       | 5    | 1.5  | +     | N/A    | Small   | 128 MiB
 mwc64             | u32    | 1     | 2       | 4    | 0.37 | +     | N/A    | Small   | 1 TiB
 mwc64x            | u32    | +     | +       | +    | 0.53 | +     | N/A    | +       | >= 8 TiB
 mwc128            | u64    | +     | +       | +    | 0.30 | +     | +      | +       | >= 16 TiB
 mwc128x           | u64    | +     | +       | +    | 0.30 | +     | +      | +       | >= 8 TiB
 mwc1616           | u32    | 8     | 12      | 14   | 0.48 | +     | N/A    |         | 16 MiB
 mwc1616x          | u32    | +     | +       | +    | 0.67 | +     | N/A    | +       | >= 8 TiB
 mwc3232x          | u64    | +     | +       | +    | 0.23 | +     | +      |         | >= 32 TiB
 pcg32             | u32    | +     | +       | +    | 0.44 | +     | N/A    | +       | >= 2 TiB
 pcg64             | u64    | +     | +       | +    | 0.28 | +     | -      | +       | >= 2 TiB
 pcg64_xsl_rr      | u64    | +     | +       | +    | 0.43 | +     | +      |         |
 philox            | u64    | +     | +       | +    | 0.85 | +     | +      | +       | >= 2 TiB
 philox32          | u32    | +     | +       | +    | 2.7  | +     | N/A    | +       | >= 2 TiB
 randu             | u32    | 19    | 34      | 37   | 0.41 | 4     | N/A    | -       | 1 KiB
 ranlux++          | u64    | +     | +       | +    | 3.9  |       |        | +       | >= 1 TiB
 r1279             | u32    | 4     | 6       | 9    | 0.47 | 2     | N/A    |         | 64 MiB
 rc4               | u32    | +     | +       | +    | 6.0  | +     | N/A    | +       | 512 GiB
 romutrio          | u64    | +     | +       | +    | 0.15 | +     | +      |         | >= 1 TiB
 rrmxmx            | u64    | +     | +       | +    | 0.14 | +     | -      |         | >= 2 TiB
 sezgin63          | u32    | +     | 1       | 3    | 3.0  | +     | N/A    |         | >= 16 TiB
 sfc8              | u32    | 3     | 7       | 13   | 1.9  | +     | N/A    |         | 128 MiB
 sfc16             | u32    | +     | +       | +    | 0.93 | +     | N/A    |         |
 sfc32             | u32    | +     | +       | +    | 0.24 | +     | N/A    |         |
 sfc64             | u64    | +     | +       | +    | 0.10 | +     | +      | +       | >= 1 TiB
 speck128          | u64    | +     | +       | +    | 3.1  | +     |        |         | >= 2 TiB
 speck128_avx      | u64    | +     | +       | +    | 0.65 | +     |        |         | >= 2 TiB
 splitmix          | u64    | +     | +       | +    | 0.19 | +     | -      |         | >= 2 TiB
 splitmix32        | u32    | 1     | 2       | 3    | 0.25 | +     | N/A    | +       | 1 GiB
 sqxor             | u64    | +     | +       | +    | 0.13 | +     | +      |         | >= 2 TiB
 sqxor32           | u32    | 1     | 2       | 3    | 0.20 | +     | N/A    | Small   | 16 GiB
 stormdrop         | u32    | +     | +       | 1    | 1.2  | +     | N/A    |         | >= 256 GiB
 superduper73      | u32    | 9     | 15      | 18   | 0.64 | 1     | N/A    |         | 32 KiB
 superduper64      | u64    | 1     | 3       | 5    | 0.35 | 1     |        |         | 512 KiB
 superduper64_u32  | u32    | +     | +       | +    | 0.70 | +     | N/A    |         | >= 2 TiB
 shr3              | u32    | 14    | 30      | 33   | 0.76 | 2     | N/A    | -       | 32 KiB
 swb               | u32    | 4     | 4       | 5    | 2.7  | 1     | N/A    |         | 128 MiB
 swblux[luxury=1]  | u32    | +     | +       | +    | 6.3  | +     | N/A    |         | 4 TiB
 swbw              | u32    | 1     | 1       | 1    | 2.8  | +     | N/A    |         | 4 GiB
 tinymt32          | u32    | 2     | 4       | 6    | 1.5  | 1     | N/A    | +       | 4 GiB
 tinymt64          | u64    | 1     | 2       | 4    | 2.7  | 1     | +      |         | 32 GiB
 threefry          | u64    | +     | +       | +    | 1.0  | +     |        | +       | >= 1 TiB
 well1024a         | u32    | 3     | 5       | 7    | 1.0  | 2     | N/A    | Small   | 64 MiB
 wyrand            | u64    | +     | +       | +    | 0.08 | +     | +      |         | >= 1 TiB
 xorshift128       | u32    | 4     | 6       | 8    | 0.41 | 2     | N/A    |         | 128 KiB
 xorshift128p      | u64    | 1     | 2       | 3    | 0.21 | 1     | +      |         | 32 GiB
 xoroshiro128p     | u64    | 1     | 2       | 3    | 0.16 | 1     | +      |         | 16 MiB
 xoroshiro128pp    | u64    | +     | +       | +    | 0.20 | +     | +      |         | >= 2 TiB
 xoroshiro1024st   | u64    | 1     | 1       | 2    | 0.33 | 1     |        |         | 128 GiB
 xoroshiro1024stst | u64    | +     | +       | +    | 0.33 | +     |        |         | >= 1 TiB
 xorwow            | u32    | 3     | 7       | 9    | 0.52 | 1     | N/A    | Small   | 128 KiB
 xsh               | u64    | 7     | 10      | 14   | 0.43 | 2     | -      | -       | 32 KiB


About `lcg64prime`: it passes BigCrush if upper 32 bits are returned, but fails it in interleaved
mode (fails test N15 `BirthdaySpacings, t = 4`).


Sensitivity of dieharder is lower than TestU01 and PractRand:

- Failed dieharder: lcg69069, lcg32prime, minstd, randu, shr3, xsh, drand48
- Passed dieharder: lcg64
