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

1. 64-bit birthday paradox test (very slow)
2. 2d Ising model test (very slow)
3. Long runs of chi2 test / monobit freq test


# Implemented algorithms

A lot of pseudorandom number generators are supplied with SmokeRand. They can
be divided into several groups:

- Cryptographically secure: chacha, isaac64, speck128, speck128_avx.
- Obsolete CSPRNG: rc4.
- Counter-based scramblers based on cryptographical primitives: philox,
  philox32, threefry.
- Lagged Fibonacci: alfib, alfib_mod, mlfib17_5, r1279
- Linear congruental: lcg64, lcg64prime, lcg96, lcg128, lcg69069, minstd,
  mwc64, mwc64x, mwc128, mwc128x, randu, seizgin63.
- Linear congruental with output scrambling: mulberry32, rrmxmx, splitmix32,
  sqxor, sqxor32, wyrand.
- Subtract with borrow: swb, swblux, swbw
- LSFR without scrambling: shr3, xsh
- LSFR with scrambling: xoroshiro128p, xoroshiro128pp, xoroshiro1024st, xorwow.
- GSFR: mt19937, tinymt32, tinymt64.
- Combined generators: kiss93, kiss99, kiss64
- Other: coveyou64, sfc64.


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
 philox            |
 philox32          |
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
 swbw              | Modification of SWB combined with 'discrete Weyl sequence'
 tinymt32          | "Tiny Mersenne Twister": 32-bit version
 tinymt64          | "Tiny Mersenne Twister": 64-bit version
 threefry          |
 well1024a         | WELL1024a: Well equidistributed long-period linear
 xoroshiro128p     |
 xoroshiro128pp    |
 xoroshiro1024st   |
 xorwow            | xorwow
 xsh               | xorshift64 generator by G.Marsaglia

# Tests results


 Algoritrhm        | Output | brief | default | full | cpb
-------------------|--------|-------|---------|------|-----------
 alfib             | u64    | 4     | -       | -    | 0.23
 alfib_mod         | u32    | +     |         |      | 0.50
 chacha            | u32    | +     |         |      | 2.0
 coveyou64         | u32    | 2     |         |      | 0.62
 isaac64           | u64    | +     | +       |      | 0.75
 kiss93            | u32    | 1     | 3       |      | 0.82
 kiss99            | u32    | +     |         |      | 1.0
 kiss64            | u64    | +     |         |      | 0.53
 lcg64             | u32    | 5     | 6       |      | 0.40
 lcg64prime        | u64    | 1     | 1       |      | 1.5
 lcg96             | u32    | +     | +       |      | 0.78
 lcg128            | u64    | +     | +       |      | 0.35
 lcg69069          | u32    | 14    | 28      |      | 0.38
 minstd            | u32    | 15    | 28      |      | 2.4
 mlfib17_5         | u32    | +     |         |      | 0.48
 mt19937           | u32    | 3     |         |      | 0.91
 mulberry32        | u32    | 1     |         |      | 0.51
 mwc64             | u32    | 1     | 2       |      | 0.37
 mwc64x            | u32    | +     |         |      | 0.53
 mwc128            | u64    | +     |         |      | 0.30
 mwc128x           | u64    | +     |         |      | 0.30
 pcg32             | u32    | +     |         |      | 0.44
 pcg64             | u64    | +     |         |      | 0.28
 philox            | u64    | +     |         |      | 0.85
 philox32          | u32    | +     |         |      | 2.7
 randu             | u32    | 16    | 29      |      | 0.41
 r1279             | u32    | 4     | 6       |      | 0.47
 rc4               | u32    | +     |         |      | 6.0
 rrmxmx            | u64    | +     | +       |      | 0.14
 seigzin63         | u32    | +     | +       | 3    | 3.0
 speck128          | u64    | +     | +       |      | 3.1
 speck128_avx      | u64    | +     | +       |      | 0.65
 splitmix32        | u32    | 1     | 1       |      | 0.25
 sqxor             | u64    | +     | +       |      | 0.13
 sqxor32           | u32    | 1     | 1       |      | 0.20
 sfc32             | u32    | +     | +       |      | 0.24 
 sfc64             | u64    | +     | +       |      | 0.10
 shr3              | u32    | 13    | 26      |      | 0.76
 swb               | u32    | 3     | 3       |      | 2.7
 swblux            | u32    | +     | +       |      | 6.3
 swbw              | u32    | +     | +       | +    | 2.8
 tinymt32          | u32    | 2     | 4       |      | 1.5
 tinymt64          | u64    | 1     | 2       |      | 2.7
 threefry          | u64    | +     | +       |      | 1.0
 well1024a         | u32    | 3     | 5       |      | 1.0
 wyrand            | u64    | +     | +       |      | 0.08
 xoroshiro128p     | u64    | 1     | 2       |      | 0.16
 xoroshiro128pp    | u64    | +     | +       |      | 0.20
 xoroshiro1024st   | u64    | 1     | 1       |      | 0.33
 xorwow            | u32    | 3     | 7       |      | 0.52
 xsh               | u64    | 6     | 8       |      | 0.43



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
