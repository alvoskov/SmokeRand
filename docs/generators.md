# Implemented algorithms

A lot of pseudorandom number generators are supplied with SmokeRand. They can
be divided into several groups:

- Cryptographical, i.e. based on block and stream ciphers: aes128, chacha,
  hc256, isaac, isaac64, kuzn, lea, rc4ok, speck128 (different modifications),
  threefish/threefish1024, xtea, xxtea etc.
- Obsolete cryptographical: DES, GOST R 34.12-2015 "Magma", RC4.
- Counter-based scramblers based on cryptographical primitives: blabla (closer
  to experimental stream cipher), philox, philox32, speck128_r16, threefry.
- Other counter-based PRNGs: jctr32, jctr64
- Lagged Fibonacci: alfib, alfib_lux, alfib_mod, mlfib17_5, lfib_par,
  lfib_ranmar, r250, r1279.
- Linear congruental: cmwc4096, drand48, lcg32prime, lcg42, lcg64, lcg64prime,
  lcg96, lcg128, lcg69069, minstd, mwc1616, mwc64, mwc128, randu, ranluxpp,
  seizgin63.
- Linear congruental with output scrambling: mwc64x, mwc128x, mwc1616x,
  mwc3232x, pcg32, pcg64, pcg64_xsl_rr.
- "Weyl sequence" (LCG with a=1) with output scrambling: mulberry32, rrmxmx,
  splitmix, splitmix32, sqxor, sqxor32, wyrand.
- "Weyl sequence" injected into reversible nonlinear transformation: sfc8,
  sfc16, sfc32, sfc64, v3b, wob2m.
- "Weyl sequence" injected into irreversible nonlinear transformation: cwg64,
  msws, stormdrop, stormdrop_old.
- Subtract with borrow: swb, swblarge, swblux, swbw.
- LSFR without scrambling: shr3, xsh, xorshift128, lfsr113, lfsr258, well1024a.
- LSFR with scrambling: xorshift128p, xorshift128pp, xoroshiro128p,
  xoroshiro128pp, xoroshiro1024st, xoroshiro1024stst, xorwow.
- GSFR: mt19937, tinymt32, tinymt64.
- Combined generators: kiss93, kiss99, kiss64, lxm_64x128, superduper73,
  superduper64, superduper96, swbmwc32, swbmwc64, xkiss etc.
- Chaotic nonlinear generators based on reversible mappings: gjrand, prvhash,
  ranval, ranval64, sfc, wob2m.
- Chaotic generators based on irreversible mappings: a5rand, msws, romutrio,
  komirand.
- Other: coveyou64, mrg32k3a, romutrio.


 Algorithm         | Description
-------------------|-------------------------------------------------------------------------
 aesni             | Hardware AES for x86-64 with 128-bit key support
 alfib             | \f$ LFib(+,2^{64},607,203) \f$
 alfib_mod         | \f$ LFib(+,2^{64},607,203) \f$ XORed by "Weyl sequence"
 blabla            | A 64-bit modification of ChaCha by J.P. Aumasson.
 chacha            | ChaCha12 CSPRNG: Cross-platform implementation
 des               | DES block cipher with 64-bit block size
 coveyou64         | COVEYOU
 isaac/isaac64     | ISAAC and ISAAC64 64-bit cryptographical PRNG by Bob Jenkins.
 kiss93            | KISS93 combined generator by G.Marsaglia
 kiss99            | KISS99 combined generator by G.Marsaglia
 kiss64            | 64-bit version of KISS
 lea128            | LEA128 block cipher in CTR mode.
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
