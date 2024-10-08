# SmokeRand
SmokeRand is a set of tests for pseudorandom number generators.

PRNGs for SmokeRand should return either 32-bit or 64-bit unsigned uniformly
distributed integers.

Implemented algorithms:

Algor

 Algoritrhm  | Description
-------------|---------------------------------------------------------------------------
 alfib       | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod   | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 chacha      | ChaCha12 CSPRNG: Cross-platform implementation 
 coveyou64   | 
 kiss93      | KISS93
 kiss99      | KISS99
 kiss64      | 64-bit version of KISS
 lcg64       | \f$ LCG(2^{64},6906969069,1) \f$ that returns upper 32 bits
 lcg128      | \f$ LCG(2^{128},18000690696906969069,1) \f$, returns upper 32/64 bits
 lcg69069    | \f$ LCG(2^{32},69069,1)\f$, returns whole 32 bits
 minstd      | \f$ LCG(2^{31} - 1, 16807, 0)\f$ "minimial standard" obsolete generator.
 mlfib17_5   | \f$ LFib(x,2^{64},17,5) \f$
 mt19937     | Mersenne twister from C++ standard library.
 mwc64       | 
 mwc64x      | MWC64X: 32-bit Multiply-With-Carry with XORing x and c
 mwc128      | 
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
2. Chi2 test for bytes and 16-bit chunks
3. Kolmogorov-Smirnov test for U(0;1)
4. Birthday spacings test with the next settings
   - 1-dimensional 32-bit (anti-LFIB)
   - 2-dimensional 64-bit 
   - 3-dimensional (takes lower 21 bits, anti-MWC)
   - 8-dimensional (takes lower 8 bits, against 64-bit LCGs)
5. Gap test: beta = 1.0/256.0, Ei_min = 10.0, alpha = 0, ngaps = 1e7
6. Matrix rank test: 512, 1024, 2048, 4096, 8192(?)
7. Linear complexity test
8. CollisionOver test:
   - 8-dimensional (takes lower 5 bits)
   - 20-dimensional (takes lower 2-3 bits)

Extra tests:

1. 64-bit birthday test (very long run)
2. Long runs of chi2 test / monobit freq test
