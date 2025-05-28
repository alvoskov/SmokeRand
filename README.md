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
  SplitMix, PCG64/64, rrmxmx, DES-CTR, MAGMA-CTR: detected by an extra
  "birthday paradox" battery.
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
- 1 GiB of RAM.

NOTE: 32-bit DOS version was tested in VirtualBox virtual machine with 512 MiB
of RAM, customization of "CollisionOver" tests allows to run SmokeRand even
at 64 MiB of RAM. The version of `express` battery for 16-bit computers
(`apps/sr_tiny.c`) can run even at 8086 with 640 KiB of RAM, tested in DosBOX.

Recommended configuration:

- C99 compiler with 128-bit integers support either through `__uint128_t` type
  (GCC, Clang) or `_umul128`/`_addcarry_u64` intrinsics (Microsoft Visual C).
  They will significantly improve performance of some PRNGs such as 128-bit
  LCGs and MWCs, RANLUX++, wyrand etc.
- 64-bit CPU; in the case of x86-64 -- support of RDTSC and RDSEED instructions
  and AVX2 instructions set by a compiler.
- Multithreading support: pthreads (POSIX threads) library of WinAPI threads.
- CMake or Ninja + Lua 5.x for compilation by means of MSVC. Or Lua 5.x for
  compilation by means of Open Watcom C.
- 16 GiB of RAM, especially for multithreaded mode and/or `birthday` battery.

The next compilers are supported: GCC (including MinGW), Clang (as zig cc), MSVC
(Microsoft Visual C) and Open Watcom C. It allows to compile SmokeRand under
Windows, UNIX-like systems and DOS. A slightly modified 16-bit version
(`apps/sr_tiny.c`) of `express` battery can be compiled even by Borland
Turbo C 2.0 or any other ANSI C (C89) compiler.

## Compilation

SmokeRand supports four software build systems: GNU Make, CMake, Ninja and
WMake (Open Watcom make). The manually written script for GNU Make is
`Makefile.gnu` and doesn't require CMake. Usage of Ninja and WMake requires
Lua 5.x interpreter for generation of `build.ninja` or `Makefile.wat` scripts
respectively (see `cfg_ninja.lua` and `cfg_wmake.lua`). Features of each
solution:

- GNU Make: supports gcc, clang (as zig cc) but not MSVC or Open Watcom.
  Has `install` and `uninstall` pseudotargets for GNU/Linux.
- Ninja: supports gcc, clang (as zig cc) and MSVC but not Open Watcom.
  Automatic resolution of `.h`-file dependencies is supported only for
  English version of MSVC.
- WMake: designed for Open Watcom C/C++, compiles for 32-bit Windows NT,
  32-bit DOS (with extenders) and 16-bit DOS. The dynamic libraries with
  PRNGs (DLLs) are shared between Windows and 32-bit DOS version.

All pseudorandom number generators are built as dynamic libraries: `.dll`
(dynamic linked libraries) for Windows and 32-bit DOS and `.so` (shared
objects) for GNU/Linux and other UNIX-like systems. 32-bit DOS version
uses a simplified custom DLL loader (see the `src/pe32loader.c` and 
`include/pe32loader.h` files) that allows to use properly compiled plugins
with PRNGs even without DOS extender with PE files support. 16-bit DOS
version (see `apps/sr_tiny.c`) doesn't support dynamic libraries at all,
tested generators should be embedded into its source code.

Notes about running tests in an environment with limited amount of RAM,
e.g. under 32-bit DOS extenders:

- `gap16_count0` may consume several MiB for gaps and frequencies tables.
  Probably RAM consumption may be significantly reduces but it may slow down
  and complicate a program for 64-bit environments.
- `bspace` uses more than 64 MiB of memory for 64-bit values. Its 32-bit
  variant needs less than 256 KiB of RAM.
- `collover` may use about 0.5 GiB of memory in most batteries. It may be
  significantly reduced but the test will be slower and less sensitive
  for 64-bit systems.
- `hamming_ot` uses relatively large (several MiB) frequency tables during
  the run.

## About implemented tests

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

- Cryptographically secure: aes128, chacha, chacha_avx (ChaCha12), hc256,
  isaac64, kuzn, lea, speck128, speck128_avx (Speck128/128).
- Obsolete CSPRNG: DES, GOST R 34.12-2015 "Magma", RC4.
- Counter-based scramblers based on cryptographical primitives: philox,
  philox32, speck128_r16, threefry.
- Lagged Fibonacci: alfib, alfib_lux, alfib_mod, mlfib17_5, lfib_par,
  lfib_ranmar, r1279.
- Linear congruental: cmwc4096, drand48, lcg32prime, lcg64, lcg64prime, lcg96,
  lcg128, lcg128_full, lcg128_u32_full, lcg69069, minstd, mwc1616, mwc64,
  mwc128, randu, ranluxpp, seizgin63.
- Linear congruental with output scrambling: mwc64x, mwc128x, mwc1616x,
  mwc3232x, pcg32, pcg64, pcg64_xsl_rr.
- "Weyl sequence" (LCG with a=1) with output scrambling: mulberry32, rrmxmx,
  splitmix, splitmix32, sqxor, sqxor32, wyrand.
- "Weyl sequence" injected into reversible nonlinear transformation: sfc8,
  sfc16, sfc32, sfc64.
- "Weyl sequence" injected into irreversible nonlinear transformation: cwg64,
  msws, stormdrop, stormdrop_old.
- Subtract with borrow: swb, swblarge, swblux, swbw.
- LSFR without scrambling: shr3, xsh, xorshift128, lfsr113, lfsr258, well1024a.
- LSFR with scrambling: xorshift128p, xorshift128pp, xoroshiro128p,
  xoroshiro128pp, xoroshiro1024st, xoroshiro1024stst, xorwow.
- GSFR: mt19937, tinymt32, tinymt64.
- Combined generators: kiss93, kiss99, kiss64, lxm_64x128, superduper73,
  superduper64, superduper64_u32.
- Other: coveyou64, mrg32k3a, romutrio.


 Algorithm         | Description
-------------------|-------------------------------------------------------------------------
 aesni             | Hardware AES for x86-64 with 128-bit key support
 alfib             | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod         | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 chacha            | ChaCha12 CSPRNG: Cross-platform implementation
 des               | DES block cipher with 64-bit block size
 coveyou64         | COVEYOU
 isaac64           | ISAAC64 64-bit CSPRNG
 kiss93            | KISS93 combined generator by G.Marsaglia
 kiss99            | KISS99 combined generator by G.Marsaglia
 kiss64            | 64-bit version of KISS
 lcg32prime        | \f$ LCG(2^{32}-5, 1588635695, 123 \f$
 lcg64             | \f$ LCG(2^{64},6906969069,1) \f$ that returns upper 32 bits
 lcg64prime        | \f$ LCG(2^{64}-59,a,0)\f$ that returns all 64 bits
 lcg96             | \f$ LCG(2^{96},a,1) \f$ that returns upper 32 bits
 lcg128            | \f$ LCG(2^{128},18000690696906969069,1) \f$, returns upper 32/64 bits
 lcg69069          | \f$ LCG(2^{32},69069,1)\f$, returns whole 32 bits
 lfsr113,lfsr258   | A combination of several LFSRs
 magma             | Magma (GOST R 34.12-2015) block cipher with 64-bit block size.
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
 sqxor             | Scrambled 64-bit "discrete Weyl sequence" (by A.L.Voskov) 
 sqxor32           | Scrambled 32-bit "discrete Weyl sequence" (by A.L.Voskov)
 superduper73      | A combination of 32-bit LCG and LFSR by G.Marsaglia.
 superduper64      | A combination of 64-bit LCG and 64-bit LFSR by G.Marsaglia.
 superduper64_u32  | `superduper64` with truncation of lower 32 bits.
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

- `express` - simplified battery that consists of 5 tests and can be
  rewritten for 16-bit platforms with 64KiB segments of data and code.
  Less powerful than `brief` but may be more sensitive than diehard
  or even dieharder.
- `brief` - a fast battery that includes a reduced set of tests, e.g.
  matrix rank tests and some tests for higher bits of generators
  are excluded.
- `default` - a more comprehensive but slower battery with extra tests
  for higher bits of generators and matrix rank tests. Other tests use
  larger samples that make them more sensitive.
- `full` - similar do default but uses larger samples. 


 Battery | Number of tests | Bytes (32-bit PRNG) | Bytes (64-bit PRNG)
---------|-----------------|---------------------|---------------------
 express | 7               | 2^26                | 2^27
 brief   | 24              | 2^35                | 2^36
 default | 41              | 2^37                | 2^38
 full    | 45              | 2^40                | 2^41


Batteries from TestU01 use the next number of values:

- SmallCrush --- 2^26 values (comparable to `express` battery)
- Crush --- 2^35 values (comparable to `default` battery)
- BigCrush -- 2^38 values (comparable to `full` battery)

However SmokeRand tests are about 5-10 times faster than tests from TestU01
due to optimized implementations that don't use floating point values and
multiplications whenever possible. Birthday spacings tests use radix sort
instead of QuickSort.

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
f\$ m={2^32}\f$, additive/subtractive lagged Fibonacci generators with small
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
 256              | 10^10 / 10B   |           |       |       |

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


# Plugins with generators

The are two ways to test PRNGs using SmokeRand: through stdin/stdout pipes and
by its implementation as a plugin, i.e. dynamic library. Usage of plugins allows
to use all CPU cores for testing. The used API is rather simple and requires
an export of only one function: `int gen_getinfo(GeneratorInfo *gi)` that should
fill the `GeneratorInfo` structure. The `include/cinterface.h` file contains
some macros that allow to avoid some boilerplate code. Numerous examples of
plugins are available in the `generators` directory. Requirements to PRNGs
source code in plugins:

- Must be freestanding, i.e. don't use any libraries including C standard
  library.
- Must be reentrant, i.e. mustn't contain any mutable static or global
  variable. PRNG state should be kept in dynamically allocated structures.

Such requirements may seem unusual and too strict. But pseudorandom number
generators should be small and fast programs that are trivially compilable
into object code. Such approach greatly reduces dynamic libraries sizes,
especially for Windows, and allows to load DLLs under DOS extenders.
There are only two problematic situations:

- Modulo operation on 64-bit numbers on 64-bit platforms and on 32-bit
  numbers on 32-bit platforms: some compilers use their runtime libraries
  for their implementation, at least in some cases. But such operations
  are usually expensive and require a manual optimization anyway.
- Operations with 64-bit numbers on some ancient 32-bit compilers such
  as Open Watcom C/C++: it may use runtime for it. In SmokeRand there is
  a workaround for it by means of some linking hacks in the autogenerated
  `Makefile.wat` file.



# Tests results

 Algorithm         | Output | express | brief | default | full | cpb  | bday64 | Grade | TestU01 | PractRand 
-------------------|--------|---------|-------|---------|------|------|--------|-------|---------|-----------
 aesni128          | u64    | +       | +     | +       | +    | 0.89 | +      | 5     |         | >= 32 TiB
 aes128(c99)       | u64    | +       | +     | +       | +    | 6.8  | +      | 5     |         | >= 32 TiB
 alfib             | u64    | 2       | 5     | 6       | 8    | 0.23 | +      | 0     | Small   | 128 MiB
 alfib_lux         | u32    | +       | 1     | 1       | 1    | 6.1  | N/A    | 3.75  | +       | 4 GiB
 alfib_mod         | u32    | +       | +     | +       | +    | 0.50 | +      | 3.5   | +       | 1 TiB
 ara32             | u32    | +       | 1     | 1       | 1    | 0.96 | +      | 2(0)  | +       | 512 MiB
 biski64           | u64    | +       | +     | +       | +    | 0.18 | +      | 4     |         | ?
 chacha            | u32    | +       | +     | +       | +    | 2.0  | +      | 5     | +       | >= 32 TiB
 chacha_avx2       | u32    | +       | +     | +       | +    | 0.7  | +      | 5     | +       | >= 16 TiB
 chacha_ctr32      | u32    | +       | +     | +       | 1    | 2.0  | -(>>10)| 0     | +       | 256 GiB
 cmwc4096          | u32    | +       | +     | +       | +    | 0.43 | +      | 4     | +       | >= 32 TiB
 coveyou64         | u32    | 1       | 3     | 4       | 4    | 0.62 | +      | 0     | Small   | 256 KiB
 cwg64             | u64    | +       | +     | +       | +    | 0.30 | +      | 4     |         | >= 16 TiB
 des-ctr           | u64    | +       | +     | +       | +    | 24   | -      | 3     |         | >= 4 TiB
 drand48           | u32    | 3       | 13    | 21      | 23/24| 0.72 | -      | 0     | -       | 1 MiB
 efiix64x48        | u64    | +       | +     | +       | +    | 0.38 | +      | 4     |         | >= 16 TiB
 isaac64           | u64    | +       | +     | +       | +    | 0.75 | +      | 5     | +       | >= 32 TiB
 flea32x1          | u32    | +       | 1     | 1       | 1    | 0.48 | +      | 2     | +       | 4 MiB
 gjrand64          | u64    | +       | +     | +       | +    | 0.32 | +      | 4     |         | >= 32 TiB
 gmwc128           | u64    | +       | +     | +       | +    | 0.72 | +      | 4(0)  |         | >= 32 TiB
 hc256             | u32    | +       | +     | +       | +    | 1.1  | +      | 5     | +       | >= 32 TiB
 hicg64_u32        | u32    | 1       | 2     | 3       | 3    | 5.4  |        | 0     |         | 32 MiB
 icg31x2           | u32    | +       | +     | +       | 1    | 87   |        |       |         | ?
 icg64             | u32    | +       | +     | +       | +    | 113  |        |       |         | ?
 icg64_p2          | u32    | 1       | 2     | 3       | 3/4  | 5.1  |        | 0     |         | 32 MiB
 kiss93            | u32    | 1       | 1     | 3       | 5    | 0.82 | +      | 2.75  | Small   | 1 MiB
 kiss99            | u32    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 kiss64            | u64    | +       | +     | +       | +    | 0.53 | +      | 4     | +       | >= 32 TiB
 kuzn              | u64    | +       | +     | +       | +    | 17   | +      | 4.5   |         | >= 1 TiB
 lcg32prime        | u32    | 1       | 13    | 24      | 26/27| 2.2  | -(>>10)| 0     | -       | 512 MiB
 lcg64             | u32    | 1       | 6     | 8       | 11   | 0.40 | +      | 0     | Small   | 16 MiB
 lcg64prime        | u64    | +       | 1     | 1       | 1    | 1.5  | -      | 0     | +-      | >= 32 TiB
 lcg96             | u32    | +       | 1     | 1       | 1    | 0.78 | +      | 3     | +       | 32 GiB
 lcg128            | u64    | +       | 1     | 1       | 1    | 0.35 | +      | 3     | +       | 64 GiB
 lcg128_full       | u64    | +       | 1     | 1       | 1    | 0.42 | +      | 3     | +       | 64 GiB
 lcg128_u32_full   | u32    | +       | +     | 1       | 1    | 0.75 | +      | 3     | +       | >= 32 TiB
 lcg69069          | u32    | 6       | 20    | 38/39   | 43   | 0.38 | -(>>10)| 0     | -       | 2 KiB
 lea128            | u32    | +       | +     | +       | +    | 5.7  | +      | 5     |         | >= 1 TiB
 lea128_avx        | u32    | +       | +     | +       | +    | 1.2  | +      | 5     |         | >= 32 TiB
 lfib_par[31+]     | u32    | 1       | 5/6   | 6/7     | 10/11| 0.70 | +      | 0     | -       | 32 MiB
 lfib_par[55+]     | u32    | 1       | 4     | 5       | 7    | 0.51 | +      | 0     | -       | 2 GiB
 lfib_par[55-]     | u32    | 1       | 4     | 5       | 7    | 0.51 | +      | 0     | -       | 2 GiB
 lfib_par[127+]    | u32    | 1       | 4     | 4       | 5    | 0.48 | +      | 0     | -/Small | 512 MiB
 lfib_par[127-]    | u32    | 1       | 4     | 4       | 5    | 0.48 | +      | 0     | -/Small | 512 MiB
 lfib_par[258+]    | u32    | 1       | 4     | 4       | 5    | 0.44 | +      | 0     | Small   | 8 GiB
 lfib_par[258-]    | u32    | 1       | 4     | 4       | 5    | 0.46 | +      | 0     | Small   | 8 GiB
 lfib_par[378+]    | u32    | 1       | 4     | 4       | 5    | 0.46 | +      | 0     | Small   | 32 GiB
 lfib_par[378-]    | u32    | 1       | 4     | 4       | 5    | 0.45 | +      | 0     | Small   | 32 GiB
 lfib_par[607+]    | u32    | 1       | 4     | 4       | 5    | 0.41 | +      | 0     | Small   | 256 GiB
 lfib_par[607-]    | u32    | 1       | 4     | 4       | 5    | 0.40 | +      | 0     | Small   | 256 GiB
 lfib_par[1279+]   | u32    | 1       | 3/4   | 3/4     | 4/5  | 0.40 | +      | 0     | Crush   | 1 TiB
 lfib_par[1279-]   | u32    | 1       | 3/4   | 3/4     | 4/5  | 0.40 | +      | 0     | Crush   | 1 TiB
 lfib_par[2281+]   | u32    | +       | 3     | 3       | 4    | 0.38 | +      | 0     | +       | 8 TiB
 lfib_par[2281-]   | u32    | 0/1     | 3     | 3       | 4    | 0.38 | +      | 0     | +       | 8 TiB
 lfib_par[3217+]   | u32    | +       | 1     | 1       | 1/2  | 0.39 | +      | 2     | +       | 16 TiB
 lfib_par[3217-]   | u32    | +       | 1     | 1       | 2/4  | 0.39 | +      | 2     | +       | 16 TiB
 lfib_par[4423+]   | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | ?
 lfib_par[4423-]   | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | ?
 lfib_par[9689+]   | u32    | +       | 1     | 1       | 1    | 0.47 | +      | 2     | +       | ?
 lfib_par[9689-]   | u32    | +       | 1     | 1       | 1    | 0.47 | +      | 2     | +       | ?
 lfib_par[19937+]  | u32    | +       | +     | 1       | 1    | 0.46 | +      | 2     | +       | ?
 lfib_par[19937-]  | u32    | +       | +     | 1       | 1    | 0.48 | +      | 2     | +       | ?
 lfib_par[44497+]  | u32    | +       | +     | 1       | 1    | 0.49 | +      | 2     | +       | ?
 lfib_par[44497-]  | u32    | +       | +     | 1       | 1    | 0.49 | +      | 2     | +       | ?
 lfib_par[110503+] | u32    | +       | +     | +       | +    | 0.52 | +      | 4     | +       | ?
 lfib_par[110503-] | u32    | +       | +     | +       | +    | 0.50 | +      | 4     | +       | ?
 lfib4             | u32    | 1       | 1     | 3       | 4    | 0.37 | +      | 3     | +       | 32 MiB
 lfib4_u64         | u32    | +       | +     | +       | +    | 0.34 | +      | 4     |         | >= 32 TiB
 lfsr113           | u32    | 2       | 3     | 5       | 7    | 1.1  | +      | 2.25  | Small   | 32 KiB 
 lfsr258           | u64    | 2       | 3     | 5       | 7    | 0.75 | +      | 2.25  | Small   | 1 MiB
 lrnd64_255        | u64    | 2       | 5     | 10      | 15   | 0.45 | +      | 0     |         | 512 KiB
 lrnd64_1023       | u64    | 2       | 3     | 5       | 7    | 0.44 | +      | 2.25  |         | 4 MiB
 lxm_64x128        | u64    | +       | +     | +       | +    | 0.42 | +      | 4     |         | >= 32 TiB
 macmarsa          | u32    | 2       | 12    | 18      | 19   | 0.67 | -(>>10)| 0     |         | 128 KiB
 magma             | u64    | +       | +     | +       | +    | 25   |        |       |         | >= 1 TiB
 magma_avx-ctr     | u64    | +       | +     | +       | +    | 7.1  | -      | 3     |         | >= 2 TiB
 magma_avx-cbc     | u64    | +       | +     | +       | +    | 7.1  | +      | 4     |         | >= 2 TiB
 melg607           | u64    | 2       | 3     | 5       | 7    | 0.73 | +      | 2.25  |         | 8 MiB
 melg19937         | u64    | +       | 3     | 3       | 3    | 0.73 | +      | 3.25  |         | 256 GiB
 melg44497         | u64    | +       | +     | 3       | 3    | 0.75 | +      | 3.25  |         | 2 TiB
 minstd            | u32    | 6       | 20    | 38      | 42   | 2.4  | -(>>10)| 0     | -       | 1 KiB
 mixmax_low32      | u32    | +       | +     | +       | +    | 1.7  | +      | 4     | +       | >= 16 TiB
 mlfib17_5         | u32    | +       | +     | +       | +    | 0.48 | +      | 4     | +       | >= 32 TiB
 mt19937           | u32    | +       | 3     | 3       | 3    | 0.50 | +      | 3.25  | Small   | 128 GiB
 mrg32k3a          | u32    | +       | +     | +       | +    | 2.5  | +      | 4     | +       | 2 TiB
 msws              | u32    | +       | +     | +       | +    | 0.72 | +      | 4     | +       | >= 16 TiB
 msws_ctr          | u64    | +       | +     | +       | +    | 0.37 | +      | 4     |         | >= 8 TiB
 msws64            | u64    | +       | +     | +       | +    | 0.41 | +      | 4     |         | >= 32 TiB
 msws64x           | u64    | +       | +     | +       | +    | 0.50 | +      | 4     |         | >= 32 TiB
 mularx64_r2       | u32    | +       | 1     | 1       | 1    | 1.5  | -      | 1     |         | 1 TiB
 mularx64_u32      | u32    | +       | +     | +       | +    | 1.7  | -      | 3     |         | >= 2 TiB
 mularx128         | u64    | +       | +     | +       | +    | 0.50 | +      | 4     |         | >= 4 TiB
 mularx128_u32     | u32    | +       | +     | +       | +    | 0.95 | +      | 4     |         | >= 32 TiB
 mularx256         | u64    | +       | +     | +       | +    | 0.67 | +      | 4     |         | >= 32 TiB
 mulberry32        | u32    | +       | 1     | 2/3     | 5    | 0.51 | -(>>10)| 0     | Small   | 512 MiB
 mwc32x            | u32    | +       | 3     | 4       | 8    | 1.5  | -(>>10)| 0     | Small   | 128 MiB
 mwc32xxa8         | u32    | +       | 1     | 4       | 10   | 1.9  | -(>>10)| 0     |         | 256 MiB
 mwc40xxa8         | u32    | +       | +     | +       | 1    | 2.1  | -(>>10)| 0     |         | 16 GiB
 mwc48xxa16        | u32    | +       | +     | +       | +    | 1.2  | +      | 4     |         | 1 TiB
 mwc64             | u32    | +       | 1     | 2       | 4    | 0.37 | -      | 0     | Small   | 1 TiB
 mwc64x            | u32    | +       | +     | +       | +    | 0.53 | +      | 4     | +       | >= 16 TiB
 mwc128            | u64    | +       | +     | +       | +    | 0.30 | +      | 4     | +       | >= 16 TiB
 mwc128x           | u64    | +       | +     | +       | +    | 0.30 | +      | 4     | +       | >= 32 TiB
 mwc128xxa32       | u32    | +       | +     | +       | +    | 0.52 | +      | 4     |         | >= 32 TiB
 mwc256xxa64       | u64    | +       | +     | +       | +    | 0.26 | +      | 4     |         | >= 32 TiB
 mwc1616           | u32    | 1       | 10/11 | 13/19   | 20   | 0.48 | -      | 0     | -/Small | 16 MiB
 mwc1616x          | u32    | +       | +     | +       | +    | 1.2  | +      | 3.5   | +       | 32 TiB
 mwc3232x          | u64    | +       | +     | +       | +    | 0.30 | +      | 4     |         | >= 32 TiB
 mwc4691           | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | 1 GiB
 mwc8222           | u32    | +       | +     | +       | +    | 0.59 | +      |       |         | >= 32 TiB
 pcg32             | u32    | +       | +     | +       | +    | 0.44 | +      | 3.5   | +       | 32 TiB
 pcg32_xsl_rr      | u32    | +       | +     | +       | +    | 0.58 | +      | 4     |         | 256 GiB
 pcg64             | u64    | +       | +     | +       | +    | 0.28 | -      | 3     | +       | >= 32 TiB
 pcg64_xsl_rr      | u64    | +       | +     | +       | +    | 0.43 | +      | 4     |         | >= 32 TiB
 philox            | u64    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 philox2x32        | u32    | +       | +     | +       | +    | 1.6  | -      | 3     |         | >= 32 TiB
 philox32          | u32    | +       | +     | +       | +    | 1.6  | +      | 4     | +       | >= 32 TiB
 ran               | u64    | +       | +     | +       | +    | 0.43 | +      | 4     |         | >= 32 TiB
 ranq1             | u64    | 1       | 1     | 3       | 6    | 0.32 | -      | 0     |         | 512 KiB
 ranq2             | u64    | +       | +     | 1       | 2    | 0.33 | +      | 3.5   |         | 2 MiB
 randu             | u32    | 6       | 22    | 40      | 44   | 0.41 | -(>>10)| 0     | -       | 1 KiB
 ranlux++          | u64    | +       | +     | +       | +    | 2.4  | +      | 4     | +       | >= 32 TiB
 ranrot_bi         | u64    | +       | +     | 1       | 2/4  | 0.33 | +      | 0     |         | 8 GiB
 ranrot32[7/3]     | u32    | +       | 3     | 5/6     | 6    | 0.58 | +      | 0     | Small   | 128 MiB
 ranrot32[17/9]    | u32    | +       | 1     | 2       | 4    | 0.68 | +      | 0     | +       | 1 GiB
 ranrot32[57/13]   | u32    | +       | +     | +       | 1    | 0.74 | +      | 2     | +       | 8 GiB
 ranshi            | u64    | +       | 2     | 7       | 8    | 0.43 | +      | 0     |         | 32 KiB
 ranshi_upper32    | u32    | +       | +     | +       | +    | 0.86 | +      | 3.5   |         | 8 TiB
 ranshi_lower32    | u32    | +       | +     | +       | +    | 0.86 | +      | 4     |         | >= 32 TiB
 ranval            | u32    | +       | +     | +       | +    | 0.31 | +      | 4(0)  | +       | >= 32 TiB
 r250              | u32    | 3       | 7     | 10      | 13   | 0.50 | +      | 0     |         | 1 MiB
 r1279             | u32    | 2       | 5     | 7       | 10   | 0.47 | +      | 0     | Small   | 64 MiB
 ranlux[0]         | u32    | 1       | 6     | 9       | 12   | 6.8  | N/A    | 0     |         | 4 MiB
 ranlux[1]         | u32    | +       | +     | 2       | 3    | 13   | N/A    | 0     |         | 4 GiB
 ranlux[2]         | u32    | +       | +     | +       | +    | 27   | N/A    |       |         | >= 128 GiB
 rc4               | u32    | +       | +     | +       | +    | 6.0  | +      | 3     | +       | 512 GiB
 rc4ok             | u32    | +       | +     | +       | +    | 6.2  | N/A    | 4.5   | +       | >= 32 TiB
 romutrio          | u64    | +       | +     | +       | +    | 0.15 | +      | 4(0)  |         | >= 1 TiB
 rrmxmx            | u64    | +       | +     | +       | +    | 0.14 | -      | 3     |         | >= 16 TiB
 sapparot          | u32    | +       | 1     | 3       | 5    | 0.70 | +      | 0     | Crush   | 8 MiB
 sapparot2         | u32    | +       | +     | +       | +    | 0.42 | +      | 3.5(0)| +       | 2 TiB
 sapparot2_64      | u64    | +       | +     | +       | +    | 0.27 | +      | 4     |         | >= 16 TiB
 sezgin63          | u32    | +       | +     | 1       | 3    | 3.0  | -      | 0     | Crush   | >= 32 TiB
 sfc8              | u32    | +       | 3     | 7       | 14   | 1.9  | -(>>10)| 0     |         | 128 MiB
 sfc16             | u32    | +       | +     | +       | +    | 0.93 | +      | 3.5(0)|         | 128 GiB(stdin32)*
 sfc32             | u32    | +       | +     | +       | +    | 0.24 | +      | 4(0)  |         | >= 4 TiB
 sfc64             | u64    | +       | +     | +       | +    | 0.10 | +      | 4     | +       | >= 16 TiB
 speck64_128       | u64    | +       | +     | +       | +    | 6.1  | -      | 3     |         | >= 4 TiB
 speck128          | u64    | +       | +     | +       | +    | 3.8  | +      | 5     |         | >= 16 TiB
 speck128_avx(full)| u64    | +       | +     | +       | +    | 0.75 | +      | 5     |         | >= 16 TiB
 speck128_avx(r16) | u64    | +       | +     | +       | +    | 0.38 | +      | 4     |         | >= 32 TiB
 splitmix          | u64    | +       | +     | +       | +    | 0.19 | -      | 3     | +       | >= 2 TiB
 splitmix32        | u32    | +       | 3     | 4       | 5/6  | 0.25 | -(>>10)| 0     | Small   | 1 GiB
 splitmix32cbc     | u32    | +       | +     | +       | +    | 2.1  | -      | 3     |         | >= 4 TiB(?)
 sqxor             | u64    | +       | +     | +       | +    | 0.13 | +      | 4     | +       | >= 16 TiB
 sqxor32           | u32    | +       | 2     | 3       | 5    | 0.20 | -(>>10)| 0     | Small   | 16 GiB
 stormdrop         | u32    | +       | +     | +       | 1    | 1.2  | +      | 0     |         | >= 32 TiB
 stormdrop_old     | u32    | +       | +     | 1       | 2    | 1.4  | +      | 3.5(0)| Small   | 1 MiB
 superduper73      | u32    | 3       | 10    | 17      | 21   | 0.64 | +      | 0     | -       | 32 KiB
 superduper64      | u64    | 1       | 1     | 3       | 5    | 0.35 | +      | 2.75  |         | 512 KiB
 superduper64_u32  | u32    | +       | +     | +       | +    | 0.70 | +      | 4     |         | >= 32 TiB
 shr3              | u32    | 2       | 15    | 32      | 36   | 0.76 | -(>>10)| 0     | -       | 32 KiB
 swb               | u32    | 1       | 5     | 6       | 8    | 2.7  | +      | 0     | Small   | 128 MiB
 swblux[luxury=1]  | u32    | +       | +     | +       | 0/1  | 6.3  | N/A    | 2     | Crush   | 4 TiB
 swblux[luxury=2]  | u32    | +       | +     | +       | +    | 9.1  | N/A    | 4     | +       | >= 2 TiB
 swblarge          | u32    | 1       | 4     | 5       | 8    | 0.56 | +      | 0     | Crush   | 512 GiB
 swbw              | u32    | +       | 1     | 1       | 1    | 2.8  | +      | 2     | +       | 4 GiB
 taus88            | u32    | 2       | 3     | 5       | 7    | 0.74 | +      | 2.25  | Small   | 32 KiB
 tinymt32          | u32    | 1       | 2     | 4       | 6    | 1.5  | +      | 0     | +       | 4 GiB
 tinymt64          | u64    | 1       | 1     | 2       | 4    | 2.7  | +      | 3     |         | 32 GiB
 threefry          | u64    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 threefry_avx      | u64    | +       | +     | +       | +    | 0.39 | +      | 4     |         | >= 2 TiB
 threefish         | u64    | +       | +     | +       | +    | 4.3  | +      | 5     |         | >= 32 TiB
 threefish_avx     | u64    | +       | +     | +       | +    | 1.3  | +      | 5     |         | >= 8 TiB
 threefry2x64      | u64    | +       | +     | +       | +    | 1.3  | +      | 4     |         | >= 16 TiB
 threefry2x64_avx  | u64    | +       | +     | +       | +    | 0.45 | +      | 4     |         | >= 32 TiB
 tylo64            | u64    | +       | +     | +       | +    | 0.17 | +      | 4     |         | >= 32 TiB
 v3b               | u32    | +       | +     | +       | +    | 0.78 | +      | 4     |         | >= 32 TiB
 wich1982          | u32    | +       | 5     | 11      | 13   | 2.3  | -      | 0     |         | 256 GiB
 wich2006          | u32    | +       | +     | +       | +    | 4.6  |        |       |         | >= 16 TiB
 well1024a         | u32    | 2       | 3     | 5       | 7    | 1.0  | +      | 2.25  | Small   | 64 MiB
 wob2m             | u64    | +       | +     | +       | +    | 0.24 | +      | 4     |         | >= 32 TiB
 wyrand            | u64    | +       | +     | +       | +    | 0.12 | +      | 4     |         | >= 32 TiB
 xorgens           | u64    | +       | +/1   | 1       | 1    | 0.41 | +      | 3.75  |         | 2 TiB
 xorshift64st      | u64    | 1       | 1     | 3       | 5    | 0.48 | -      | 1.75  |         | 512 KiB
 xorshift128       | u32    | 2       | 5     | 7/8     | 9    | 0.41 | +      | 0     | -       | 128 KiB
 xorshift128p      | u64    | 1       | 1     | 2       | 3    | 0.26 | +      | 3.25  |         | 32 GiB
 xorshift128pp     | u64    | +       | +     | +       | +    | 0.29 | +      | 4     |         | >= 16 TiB
 xorshift128pp_avx | u64    | +       | +     | +       | +    | 0.15 | +      | 4     |         | >= 1 TiB
 xoroshiro128      | u64    | 2       | 3     | 5       | 7    | 0.27 | +      | 2.25  |         | 256 KiB
 xoroshiro128p     | u64    | 1       | 1     | 2       | 3    | 0.16 | +      | 3.25  |         | 16 MiB
 xoroshiro128pp    | u64    | +       | +     | +       | +    | 0.26 | +      | 4     |         | >= 32 TiB
 xoroshiro128pp_avx| u64    | +       | +     | +       | +    | 0.16 | +      | 4     |         | >= 1 TiB
 xoroshiro1024st   | u64    | 1       | 1     | 1       | 2    | 0.33 | +      | 3.5   |         | 128 GiB
 xoroshiro1024stst | u64    | +       | +     | +       | +    | 0.33 | +      | 4     | +       | >= 16 TiB
 xorwow            | u32    | 1       | 3     | 7       | 9    | 0.52 | +      | 0     | Small   | 128 KiB
 xoshiro128p       | u32    | 1       | 1     | 2       | 4    | 0.38 | +      | 3     | +       | 8 MiB
 xoshiro128pp      | u32    | +       | +     | +       | +    | 0.42 | +      | 4     | +       | >= 16 TiB
 xsh               | u64    | 2       | 9     | 14      | 18   | 0.43 | -      | 0     | -       | 32 KiB
 xtea              | u64    | +       | +     | +       | +    | 27   | -      | 3     |         | ?
 xtea_avx(ctr)     | u64    | +       | +     | +       | +    | 2.3  | -      | 3     |         | >= 32 TiB
 xtea_avx(cbc)     | u64    | +       | +     | +       | +    | 2.3  | +      | 4     |         | >= 1 TiB
 xxtea128          | u32    | +       | +     | +       | +    | 18   | +      | 4.5   |         | ?
 xxtea128_avx      | u32    | +       | +     | +       | +    | 2.7  | +      | 4.5   |         | >= 32 TiB
 xxtea256          | u32    | +       | +     | +       | +    | 12   | +      | 4.5   |         | ?
 xxtea256_avx      | u32    | +       | +     | +       | +    | 1.9  | +      | 4.5   |         | >= 32 TiB
 ziff98            | u32    | +       | 3     | 3       | 3    | 0.47 |        | 3.25  |         | 32 GiB

Performance estimation for some 64-bit generators

 Generator                | ising    | bday64   | usphere  | default(!)
--------------------------|----------|----------|----------|----------
 sfc64                    | 00:10:56 | 00:03:41 | 00:03:36 | 00:03:05
 xoroshiro128++           | 00:11:04 | 00:03:13 | 00:03:45 | 00:03:02
 wyrand                   | 00:11:01 | 00:03:05 | 00:03:23 | 00:03:03
 mwc256xxa64              | 00:11:28 | 00:04:01 | 00:03:43 | 00:03:13
 tylo64                   | 00:11:11 | 00:03:52 | 00:04:00 | 00:03:10
 wob2m                    | 00:11:48 | 00:03:17 | 00:03:21 | 00:03:17
 pcg64_xsl_rr             | 00:12:01 | 00:04:22 | 00:05:22 | 00:03:30
 speck128/128-vector-full | 00:15:45 | 00:06:53 | 00:06:50 | 00:04:06
 aesni                    | 00:15:43 | 00:08:32 | 00:09:44 | 00:04:14
 speck128/128-scalar      | 00:31:18 | 00:24:56 | 00:23:19 | 00:07:46
                          |          |          |          |


Performance estimation for some 32-bit generators

 Generator                | ising    | bday64   | usphere  | default
--------------------------|----------|----------|----------|----------
 kiss99                   | 00:12:48 |          |          |
 chacha:c99               | 00:19:50 |          |          |


Note about `mt19937` and `philox`: speed significantly depends on gcc optimization settings:
e.g. changing `-O2` to `-O3` speeds up `mt19937` but slows down `philox`; gcc 10.3.0 (tdm64-1).

Note about `mrg32k3a`: it fails the `FPF-14+6/16:cross` test from PractRand at 4 TiB sample.
The failure is systematic and reproducible.

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

Note about `pcg32`: it fails the TMFn test from PractRand at 64 TiB!

Run 1:

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 105157 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+11):wl                     R= +25.4  p~=   2e-8    very suspicious
      ...and 346 test result(s) without anomalies

Run 2:

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 98476 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+12):wl                     R= +21.1  p~=   2e-6    mildly suspicious
      ...and 346 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 64 terabytes (2^46 bytes), time= 196881 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+11):wl                     R= +25.5  p~=   1e-8    very suspicious
      TMFn(2+12):wl                     R= +47.6  p~=   1e-21     FAIL !!
      TMFn(2+13):wl                     R= +20.6  p~=   3e-6    unusual
      ...and 349 test result(s) without anomalies


About `lcg64prime`: it passes BigCrush if upper 32 bits are returned, but
fails it in interleaved mode (fails test N15 `BirthdaySpacings, t = 4`).

About `mwc1616x`: it passes until 32 TiB but it fails the BCFN test at 64 TiB.

Run 1:

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

Run 2:

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 52634 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R=  +9.1  p =  2.0e-4   unusual
      BCFN(2+28,13-8,T)                 R= +30.0  p =  1.5e-8   unusual
      BCFN(2+29,13-9,T)                 R= +32.8  p =  1.7e-8   unusual
      ...and 336 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 106496 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R=  +8.9  p =  2.7e-4   unusual
      BCFN(2+3,13-0,T)                  R= +10.5  p =  3.9e-5   unusual
      BCFN(2+21,13-3,T)                 R= +15.4  p =  4.1e-7   unusual
      BCFN(2+27,13-7,T)                 R= +26.9  p =  1.0e-8   unusual
      BCFN(2+28,13-8,T)                 R= +31.5  p =  6.1e-9   unusual
      BCFN(2+29,13-8,T)                 R= +40.0  p =  4.2e-11  suspicious
      BCFN(2+30,13-9,T)                 R= +43.4  p =  7.2e-11  suspicious
      BCFN(2+31,13-9,T)                 R= +44.0  p =  5.2e-11  suspicious
      FPF-14+6/16:all                   R=  +5.9  p =  4.7e-5   mildly suspicious
      ...and 338 test result(s) without anomalies
    
    rng=RNG_stdin32, seed=unknown
    length= 64 terabytes (2^46 bytes), time= 213220 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +15.5  p =  8.1e-8   very suspicious
      BCFN(2+1,13-0,T)                  R= +11.4  p =  1.2e-5   mildly suspicious
      BCFN(2+2,13-0,T)                  R=  +9.4  p =  1.4e-4   unusual
      BCFN(2+3,13-0,T)                  R=  +9.2  p =  1.8e-4   unusual
      BCFN(2+4,13-0,T)                  R= +14.0  p =  5.0e-7   suspicious
      BCFN(2+5,13-0,T)                  R= +11.7  p =  8.6e-6   mildly suspicious
      BCFN(2+6,13-0,T)                  R= +12.9  p =  1.9e-6   mildly suspicious
      BCFN(2+7,13-0,T)                  R= +11.9  p =  6.5e-6   mildly suspicious
      BCFN(2+8,13-0,T)                  R= +13.6  p =  7.9e-7   mildly suspicious
      BCFN(2+10,13-0,T)                 R= +11.3  p =  1.4e-5   unusual
      BCFN(2+12,13-0,T)                 R= +12.2  p =  4.8e-6   unusual
      BCFN(2+19,13-1,T)                 R= +15.0  p =  1.6e-7   unusual
      BCFN(2+20,13-2,T)                 R= +19.4  p =  1.5e-9   suspicious
      BCFN(2+21,13-3,T)                 R= +28.5  p =  2.7e-13   VERY SUSPICIOUS
      BCFN(2+22,13-3,T)                 R= +23.9  p =  4.3e-11  very suspicious
      BCFN(2+23,13-4,T)                 R= +31.9  p =  7.2e-14   VERY SUSPICIOUS
      BCFN(2+24,13-5,T)                 R= +39.2  p =  1.6e-15    FAIL
      BCFN(2+25,13-5,T)                 R= +38.1  p =  4.3e-15    FAIL
      BCFN(2+26,13-6,T)                 R= +44.1  p =  1.5e-15    FAIL
      BCFN(2+27,13-6,T)                 R= +46.9  p =  1.6e-16    FAIL
      BCFN(2+28,13-7,T)                 R= +62.9  p =  1.5e-19    FAIL !
      BCFN(2+29,13-8,T)                 R= +80.5  p =  2.2e-21    FAIL !
      BCFN(2+30,13-8,T)                 R= +74.2  p =  9.1e-20    FAIL !
      BCFN(2+31,13-9,T)                 R= +91.5  p =  1.1e-21    FAIL !
      DC6-9x1Bytes-1                    R=  +7.0  p =  2.6e-3   unusual
      FPF-14+6/16:all                   R= +11.3  p =  3.9e-10   VERY SUSPICIOUS
      ...and 326 test result(s) without anomalies

Sensitivity of dieharder is lower than TestU01 and PractRand:

- Failed dieharder: lcg69069, lcg32prime, minstd, randu, shr3, xsh, drand48, lfib31
- Passed dieharder: lcg64, lfib(55,24,+,up32), lfib(607,203,+,up32), swb, xorwow

Sensitivity of ENT for 1 GiB sample (at 4 GiB it fails on Win32 due to `long`
and/or `int` data types overflow) is much lower than `express` battery. The most
sensitive test from ENT is the byte frequency test. It also includes entropy
test, arithmetic mean, Monte-Carlo pi and serial correlation tests. But they
are less sensitive, e.g. entropy test catches only randu.

- Failed ENT: randu, lcg69069, drand48, shr3
- Passes ENT: lcg32prime, lcg64, lfib31, swb

# Versions history

01.05.2025: SmokeRand 0.34

- Several new generators.
- The new `hamming_distr` test that is based on calculation of n-bit blocks
  Hamming weights and analysis of their distribution. Also processes pairwise
  XORs of that blocks. It was added to the `brief`, `default` and `full`
  batteries.
- 

10.04.2025: SmokeRand 0.33

- All generators are either portable or at least have portable versions.
- A portable implementation of 128-bit arithmetics subset was added to the
  header files (see `include/smokerand/int128defs.h`). It is designed mainly
  for 128-bit LCGs.
- A new nonlinear generator `wob2m` by Bob Jenkins.
- A portable software implementation of AES was added.

28.03.2025: SmokeRand 0.32

- The variants of the `xxtea` algorithm with 128-bit and 256-bit blocks
  were added (selected by means of the `--param=mode` command line option).
  Options: `scalar-128`, `avx2-128`, `scalar-256`, `avx2-256`.

26.03.2025: SmokeRand 0.31

- The `mwc32xxa8`, `mwc40xxa8`, `mwc48xxa16`, `mwc128xxa32`, `mwc256xxa16`,
  `threefry_avx`, `xtea` and `xtea_avx` generators were added.
- `threefry` and `threefry_avx` generators now support `Threefry` and
  `Threefish` modes as `--param=mode`.
- Improved debug output in the `gap` and `gap16_count0` tests.

18.03.2025: SmokeRand 0.30

- `testgens` removed because 32-bit DOS versions of SmokeRand supports DLLs now.
  And for other platforms without shared library support the examples from
  `apps/test_crand.c` and `apps/test_cpp11.cpp` are much more useful.
- `apps/test_crand.c`: an example of PRNG testing using static linking with
  SmokeRand core.
- `brief` battery extended with the `mod3` test. It is fast but catches some
  nonlinear generators such as `ara32` and `flea32x1`.
- `radixsort64`: switches now to `quicksort64` if there is not enough memory
  for the internal buffers.
- 64-bit birthday paradox uses the custom `quicksort64` instead of `qsort`:
  a slight improvement in performance was achieved.
- Entropy module ror64 bugfix.

16.03.2025: SmokeRand 0.29

- `express` battery extended: `bspace4_8d` and `bspace8_4d` tests were added.
  It allows to detect flaws in `xorwow` and `mwc1616` generators respectively.
  The `sr_tiny` executable (C89 implementation of `express` battery) was also
  upgraded.
- DLL support for 32-bit DOS version was added by means of implementation of
  a simplified PE32 loader.
- Improved makefiles for Open Watcom C: freestanding DLLs, 32-bit and 16-bit
  DOS versions.
- Improvements and simplification in a multithreaded battery launcher: dynamic
  assignment of tests to threads, randomization. Estimated times and RAM
  usage intensity were removed from tests info: they were not relevant and
  accurate anyway.
- Some bugfixes in makefiles, especially for zig cc (clang). Now freestanding
  libraries linking is also supported for zig cc / clang.

10.03.2025: SmokeRand 0.28

- `gap16_count0`: "not enough memory" messages added.
- `Makefile.d32` file for building a simplified version of SmokeRand 
  for 32-bit DOS extenders using Open Watcom C.
- `testgens`: a simplified version of SmokeRand for platforms without DLL
  support. Mainly a hack for DOS version.
- `xoshiro128+` and `xoshiro128++` generators were added.
- Internal self-tests for some LFSR generators were added.

07.03.2025: SmokeRand 0.27

- Improved computation of p-values for the linear complexity test.
- Bugfix in the rrmxmx generator.
- Test vectors for some generators.
- Generators source files are renames (`_shared` suffix excluded).

05.03.2025: SmokeRand 0.26

- Multithreading support was transferred to a separate file.
- Improved threads IDs printing.
- `make install`, `make uninstall` and deb package support
  for UNIX-like systems was added.

04.03.2025: SmokeRand 0.25

- lrnd64 (a modification of lrnd32-1023) generator.
- Extension of Lua scripts for Ninja build system.
- `freq` battery now shows bytes/words that caused failure.
- Open Watcom C support for Windows NT target. It is always a 32-bit build
  and may have performance issues, especially with modern CSPRNGs. But it
  was successfully tested in FreeDOS (HX DOS Extender).

25.02.2025: SmokeRand 0.24.

- man page draft added.
- `aesni` fixes for clang.
- PractRand results updated for some generators.
- lua script for build.ninja.
- A new `--report-brief` key for brief final reports.

12.02.2025: SmokeRand 0.23.

- efiix64x48, DES-CTR, xorgens were added.
- In `full` battery number of bits in linearcomp tests was increased to
  1 million, mainly due to experiments with lower bits of xorgens and
  other scrambled LFSRs.

09.02.2025: SmokeRand 0.22.

- LXM PRNG
- Scripted version of the `ising` battery.
- Refactoring.

08.02.2025: SmokeRand 0.21.

- Scripted versions of all four general purpose batteries: `express`, `brief`,
  `default`, `full`.
- Refactoring of C versions of batteries.

07.02.2025: SmokeRand 0.20.

- Preliminary support of user-defined batteries stored in text files.
- Philox2x32 and 'squares' CBPRNG by B.Widynski.
- Refactoring that allowed to get rid of a lot of small functions in the
  battery description (support of user-defined data for tests was added).

02.01.2025: SmokeRand 0.19.

- CSPRNG based on Magma GOST R 34.12-2015 block cipher (CTR and CBC modes).

31.01.2025: SmokeRand 0.18.

- segfault was fixed in the `gap16_count0` test and memory leaks --- in
  the `nbit_words_freq_test` test.
- Small refactoring of the `gap16_count0` test.
- AVX2 versions of xoroshiro128++ and xorshift128+ (new PRNG)
- Improved seed generator, uses more entropy sources and machine ID.

29.01.2025: SmokeRand 0.17.

- Birthday test now works with 32-bit generators.
- New PRNGs including Threefry2x64x20 (scalar and AVX2 versions).

23.12.2024: SmokeRand 0.16.

- Speck128/128 with reduced number of rounds.
- `dos16` renamed to `express`.
- Correct program termination in the case of PRNG example allocation failure.
  It may occur in the case of incorrect optional parameter.

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
 74 = 64 + 10   | 512 GiB; 512 GiB
 76 = 64 + 12   | 1 TiB
 77 = 64 + 13   | 1 TiB
 78 = 64 + 14   | NOTE: doesn't fail at 32 TiB!
 79 = 64 + 15   | NOTE: doesn't fail at 1 TiB!

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

Approximation in GNU Octave:

    tr = [32 64 96 128]'
    s = [8*2^20 512*2^20 32*2^30 2*2^40]'
    y = log2(s)
    b = X \ y

The obtained approximation for sample size (32-bit output) n and number of
truncated bits t:

    n = 2^(17 + 3*t/16) => t = (log2(n) - 17) * 16 / 3

So for 1 PiB t = 176, for 8 PiB t = 192 that correspond to the 256-bit LCG that
retuns upper 64 bits. Of course, 1 PiB is only a minimalistic lower boundary
for a general purpose PRNG that is used to draw a border between seriously
flawed and barely usable LCGs with \f$ m = 2 ^ k \f$ modulo.

