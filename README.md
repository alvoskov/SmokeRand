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
- Multithreading support by means of POSIX threads.

Existing solutions:

1. TestU01 https://dl.acm.org/doi/abs/10.1145/3447773
2. PractRand
3. Ent
4. Dieharder

Requirements:

- C99 compiler, some PRNGs will require 128-bit integers either through
  `__uint128_t` type (GCC, Clang) or `_umul128`/`_addcarry_u64` intrinsics
  (Microsoft Visual C)
- pthreads (POSIX threads) library.
- 64-bit CPU, x86-64 with RDRAND and AVX2 support is recommended.
- 4GiB of RAM minimal, 16GiB recommended.
- CMake.

Implemented tests:

1. Monobit frequency test.
2. Chi2 frequency test for bytes and 16-bit chunks.
3. Birthday spacings test.
4. Gap test
5. Matrix rank test.
6. Linear complexity test.
7. CollisionOver test.

Extra tests:

1. 64-bit birthday test (very slow)
2. 2d ising model test (very slow)
3. Long runs of chi2 test / monobit freq test


# Implemented algorithms

A lot of pseudorandom number generators are supplied with SmokeRand. They can
be divided into several groups:

- Cryptographically secure: chacha, speck128, speck128_avx.
- Obsolete CSPRNG: rc4.
- Lagged Fibonacci: alfib, alfib_mod, mlfib17_5, r1279
- Linear congruental: lcg64, lcg64prime, lcg96, lcg128, lcg69069, minstd,
  mwc64, mwc64x, mwc128, mwc128x, randu, seizgin63.
- Linear congruental with output scrambling: mulberry32, rrmxmx, splitmix32,
  sqxor, sqxor32.
- Subtract with borrow: swb, swblux, swbw
- LSFR without scrambling: shr3, xsh
- LSFR with scrambling: xoroshiro128p, xoroshiro128pp, xoroshiro1024st, xorwow.
- GSFR: mt19937, tinymt32, tinymt64.
- Combined generators: kiss93, kiss99, kiss64
- Other: coveyou64, sfc64.


 Algoritrhm        | Description
-------------------|-------------------------------------------------------------------------
 alfib             | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod         | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 chacha            | ChaCha12 CSPRNG: Cross-platform implementation
 coveyou64         | COVEYOU
 kiss93            | KISS93 combined generator by G.Marsaglia
 kiss99            | KISS99 combined generator by G.Marsaglia
 kiss64            | 64-bit version of KISS
 lcg64             | \f$ LCG(2^{64},6906969069,1) \f$ that returns upper 32 bits
 lcg64prime        | \f$ LCG(2^{64}-59,a,0)\f$ that returns all 64 bits
 lcg96             | \f$ LCG(2^{96},a,1) \f$ that returns upper 32 bits
 lcg128            | \f$ LCG(2^{128},18000690696906969069,1) \f$, returns upper 32/64 bits
 lcg69069          | \f$ LCG(2^{32},69069,1)\f$, returns whole 32 bits
 minstd            | \f$ LCG(2^{31} - 1, 16807, 0)\f$ "minimial standard" obsolete generator.
 mlfib17_5         | \f$ LFib(x,2^{64},17,5) \f$
 mt19937           | Mersenne twister.
 mulberry32        | Mulberry32 generator.
 mwc64             | Multiply-with-carry generator with 64-bit state
 mwc64x            | MWC64X: modification of MWC64 that returns XOR of x and c
 mwc128            | Multiply-with-carry generator with 64-bit state
 mwc128x           | MWC128X: similar to MWC64X but x and c are 64-bit
 pcg32             | Permuted Congruental Generator (32-bit version, 64-bit state)
 pcg64             | Permuted Congruental Generator (64-bit version, 64-bit state)
 randu             | \f$ LCG(2^{32},65539,1) \f$, returns whole 32 bits
 r1279             | \f$ LFib(XOR, 2^{32}, 1279, 1063) \f$ generator.
 rc4               | RC4 obsolete CSPRNG (doesn't pass PractRand)
 rrmxmx            | Modified SplitMix PRNG with improved output function
 seigzin63         | \f$ LCG(2^{63}-25,a,0) \f$
 speck128          | Speck128/128 CSPRNG.
 speck128_avx      | Modification of `speck128` for AVX2.
 splitmix32        | 32-bit modification of SplitMix
 sqxor             |
 sqxor32           |
 sfc64             | "Small Fast Chaotic 64-bit" PRNG by Chris Doty-Humphrey
 shr3              | xorshift32 generator by G.Marsaglia
 swb               | 32-bit SWB (Subtract with borrow) generator by G.Marsaglia
 swblux            | Modification of SWB with 'luxury levels' similar to RANLUX
 swbw              | Modification of SWB combined with 'discrete Weyl sequence
 tinymt32          | "Tiny Mersenne Twister": 32-bit version
 tinymt64          | "Tiny Mersenne Twister": 64-bit version
 xoroshiro128p     |
 xoroshiro128pp    |
 xoroshiro1024st   |
 xorwow            | xorwow
 xsh               | xorshift64 generator by G.Marsaglia


# Modifications of birthday spacings test

The birthday test generates input values using the next algorithm:

1. Get `ndim` pseudorandom values and take the lower `nbits` bits from
   each value.
2. Concatenate them as `x_n | x_{n-1} | ... | x_{1}`.

 Name        | nbits | ndim | Detected PRNGs
-------------|-------|------|------------------------------------------------------
 bspace64_1d | 64    | 1    | 64-bit LCG with prime modulus
 bspace32_1d | 32    | 1    | Additive LFib PRNGs and LCGs with m = 2^{96}
 bspace32_2d | 32    | 2    | LCGs
 bspace21_3d | 21    | 3    | 64-bit MWC generators.
 bspace16_4d | 16    | 4    | 
 bspace8_8d  | 8     | 8    | LCGs with m = 2^{64}

# Modifications of collision over test

 Name          | nbits | ndim | Detected PRNGs
---------------|-------|------|--------------------
 collover20_2d | 20    | 2    |
 collover13_3d | 13    | 3    |
 collover8_5d  | 8     | 5    |
 collover5_8d  | 5     | 8    |


# Modifications of linear complexity tests

 Name            | Bit for 32/64-bit PRNG
-----------------|------------------------
 linearcomp_high | 31/63
 linearcomp_mid  | 15/31
 linearcomp_low  | 0/0


# Modifications of matrix rank test

 Name                 | n      | nbits
----------------------|--------|--------
 matrixrank_4096      | 4096   | 32/64
 matrixrank_4096_low8 | 4096   | 8
 matrixrank_8192      | 8192   | 32/64
 matrixrank_8192_low8 | 8192   | 8
