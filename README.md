# SmokeRand
SmokeRand is a set of tests for pseudorandom number generators. Tested
generators should return either 32-bit or 64-bit unsigned uniformly distributed
unsigned integers.

Implemented algorithms:

 Algoritrhm  | Description
-------------|---------------------------------------------------------------------------
 alfib       | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod   | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 chacha      | ChaCha12 CSPRNG: Cross-platform implementation 
 coveyou64   | COVEYOU
 kiss93      | KISS93
 kiss99      | KISS99
 kiss64      | 64-bit version of KISS
 lcg64       | \f$ LCG(2^{64},6906969069,1) \f$ that returns upper 32 bits
 lcg64prime  | \f$ LCG(2^{64}-59,a,0)\f$ that returns all 64 bits
 lcg96       | \f$ LCG(2^{96},a,1) \f$ that returns upper 32 bits
 lcg128      | \f$ LCG(2^{128},18000690696906969069,1) \f$, returns upper 32/64 bits
 lcg69069    | \f$ LCG(2^{32},69069,1)\f$, returns whole 32 bits
 minstd      | \f$ LCG(2^{31} - 1, 16807, 0)\f$ "minimial standard" obsolete generator.
 mlfib17_5   | \f$ LFib(x,2^{64},17,5) \f$
 mt19937     | Mersenne twister from C++ standard library.
 mwc64       | MWC64
 mwc64x      | MWC64X: 32-bit Multiply-With-Carry with XORing x and c
 mwc128      | MWC64
 mwc128x     | MWC128X: similar to MWC64X but x and c are 64-bit
 pcg32       | Permuted Congruental Generator (32-bit version, 64-bit state)
 pcg64       | Permuted Congruental Generator (64-bit version, 64-bit state)
 randu       | \f$ LCG(2^{32},65539,1) \f$, returns whole 32 bits
 rc4         | RC4 obsolete CSPRNG (doesn't pass PractRand)
 rrmxmx      | Modified SplitMix PRNG with improved output function
 seigzin63   | \f$ LCG(2^{63}-25,a,0) \f$
 speck128    | Speck128/128 CSPRNG
 sfc64       | "Small Fast Chaotic 64-bit" PRNG by 
 xorwow      | xorwow
 xsh         | xorshift64



Desired tests:

1. Monobit frequency test
2. Chi2 frequency test for bytes and 16-bit chunks
3. Birthday spacings test
4. Gap test
5. Matrix rank test: 4096, 8192
6. Linear complexity test
7. CollisionOver test.

Extra tests:

1. 64-bit birthday test (very long run)
2. Long runs of chi2 test / monobit freq test


# Modifications of birthday test

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


# Modifications of matrix rank test

 Name                 | n      | nbits
----------------------|--------|--------
 matrixrank_4096      | 4096   | 32/64
 matrixrank_4096_low8 | 4096   | 8
 matrixrank_8192      | 8192   | 32/64
 matrixrank_8192_low8 | 8192   | 8


# Modifications of linear complexity tests

 Name            | Bit for 32/64-bit PRNG
-----------------|------------------------
 linearcomp_high | 31/63
 linearcomp_mid  | 15/31
 linearcomp_low  | 0/0
