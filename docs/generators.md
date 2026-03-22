# Implemented algorithms

A lot of pseudorandom number generators are supplied with SmokeRand. They can
be divided into several groups.

## Cryptographical generators

These generators are based on block or stream ciphers. Of course our
implementations are not designed for security related applications and mustn't
be used for such purposes. Some of them have platform-dependend command line
options (like `--param=8-avx2` for `chacha`) to activate fast SIMD
platform-dependent implementations.

AES (`aes128`), ChaCha8/12/20 (`chacha`), ThreeFish1024-CTR (`threefish1024`)
and Speck128/128 (`speck128`) can be used as reference PRNGs for statistical
tests calibration, simulations etc. Their hardware accelerated implementations
are fast (often around 0.6-1.5 cpb) and give the same results as scalar ones.

- 128-bit block ciphers: aes128, kuzn, lea, speck128, xxtea etc.
- Block ciphers with large block sizes: ThreeFish-1024, xxtea.
- Stream ciphers with arbitrary access: chacha (widely used), blabla
  (experimental)
- Other stream ciphers: hc256, isaac, isaac64, rc4ok
- Obsolete cryptogrpahical: DES, GOST R 34.12-2015 "Magma", RC4.

## Counter based generators (CBPRNG)

CBPRNG resemble block ciphers but use weaker output functions. Examples:

- Widely used: philox, philox32, threefry.
- Experimental (obtained by A.L. Voskov from existing solutions):
  speck128_r16 (Speck128/128 with halved number of rounds), jctr32, jctr64
  (based on very experimental block cipher by Bob Jenkins).
- Counter-based with non-bijective output function: msws_ctr.
- "Weyl sequence" (LCG with a=1 and c<>0) with output scrambling: mulberry32,
  ranhash, rrmxmx, splitmix, splitmix32, sqxor, sqxor32, wyrand.

## Lagged Fibonacci generators

- Additive/subtractive generators with two lags: alfib, alfib_lux, lfib_par.
- Multiplicative generator: mlfib17_5
- Additive generators with several lags: alfib64x5, lfib4.
  

## Linear congruential generators

- With \f$m = 2^k\f$: drand48, lcg42, lcg64, lcg96, lcg128, lcg69069, randu.
- With prime m: lcg32prime, lcg61prime, lcg64prime, lcg127prime, lcg128prime,
  minstd, ranluxpp, seizgin63.
- MWC (multiply-with-carry): cmwc4096, cmwc4827, mwc4691, mwc8222, mwc64,
  mwc128, mwc192, mwc256, gmwc128.
- MWC with several lags: rwc32sm, mall32 (by G. Marsaglia), rwc32, rwc64,
  rwc64, mall64, mall16ex (modifications made by A.L. Voskov).
- MWC with output scrambling: mwc64x, mwc128x.
- PCG (LCG with output scramblers):  pcg32, pcg64, pcg64_64, pcg64_dxsm,
  pcg64_xsl_rr, pcg128 etc.
- Other scrambled LCGs: lcg64bd, lcg64sc, lcg64sc2, lcg64bd.
- Subtract with borrow: cswb4288, cswb4288_64, ranlux48, swb, swblarge, swblux,
  swblux64.

## LFSR/GFSR

- Mersenne Twister: mt19937, mt19937_64.
- Scrambled LFSRs from MT19937 developers: tinymt32, tinymt64 ("Tiny Mersenne
  Twisters"), xsadd.
- Some large LFSRs: melg607, melg19937, melg44497, well1024a.
- LFSRs by M.V. Iakobovskii: lrnd64_255, lrnd_1023.
- xorshift: shr3 (xorshift32), xsh (xorshift64), xorshift128.
- xorshift-like with scramblers: xorshift128p, xorshift128pp, xoroshiro128aox,
  xoroshiro128p, xoroshiro128pp, xoroshiro1024st, xoroshiro1024stst.
- xorrot family (by A.L. Voskov): xorrot32, xorrot64, xorrot64w32, xorrot64w16,
  xorrot128, xorrot128w32, xorrot256.
- xorrot with output scramblers (by A.L. Voskov): xorrot64mn, xorrot64mrt,
  xorrot64w8sc, xorrot64w16nn, xorrot64w32mn, xorrot128mn, xorrot128w32mrt,
  xorrot256mrt.
- Combined LFSRs: lfsr113, lfsr258, taus88.
- XOR based lagged Fibonacci: r250, r1279, ziff98.

## Combined generators

- KISS family (classic) kiss93, kiss96, kiss99, kiss03, kiss64.
- KISS family without multiplication: jkiss32 (the original version by
  G.Marsaglia), xkiss8_awc, xkiss16_awc, xkiss16sh_awc, xkiss32_awc_rot,
  xkiss32sh, xkiss64_awc (modifications made by A.L. Voskov)
- KISS-style generators from "Numerical Recipes" (3rd edition): ran, ranlim32.
- KISS with extended state: kiss4691, kiss11_32, kiss11_64, skiss32, skiss64.
- KISS with improved output functions and xorrot LFSR (by A.L. Voskov):
  kiss32rot, kiss64rot.
- SuperDuper generators by G. Marsaglia: superduper73, superduper64,
  superduper96. Also ranq2 from "Numerical Recipes" (3rd edition).
- tf0duper32 and tf0duper64: SuperDuper modifications developed by A.L. Voskov.
  They use Klimov-Shamir T-function instead of LCG, xorrot instead of xorshift.
  Also their output functions are more sophisticated than in the original
  SuperDuper.
- LXM (KISS-like with output scrambler): lxm_64x128.
- Two MWC: mwc1616, mwc1616x, mwc3232x.
- Multipicative LFIB + MWC: ultra, ultra64.
- Additive LFIB + discrete Weyl sequence: lfib_ranmar.
- SWB + MWC: swbmwc32, swbmwc64.
- SWB + discrete Weyl sequence: swbw.
- mrg32k3a
- ranecu and ran2
- xorshift128 + discrete Weyl sequence: xorwow
- Wichmann-Hill generators: wich1982, wich2006.
- alfib_mod

## Chaotic nonlinear generators

- Reversible mappings with linear part (discrete Weyl sequence): biski8,
  biski16, biski32, biski64, efiix64x48, gjrand8, gjrand16, gjrand32, gjrand64,
  prvhash64cw, sfc8, sfc16, sfc32, sfc64, tychei64w,
  v3b, wob2m, zibri64ex, zibri128, zibri128ex, zibri192, zibri192ex.
- Reversible mappings without linear part: flea32x1, ranval, ranval64,
  prvhash64c, ranrot_bi, romutrio(?), romuduojr(?), tychei, tychei64.
- Irreversible mappings with linear part: a5randw, msws, cwg64, komirandw,
  stormdrop.
- Irreversible mappings without linear part: a5rand, komirand.

## Other nonlinear generators

These generators are based on some nonlinear mappings with proven period.

- bbs64 (a toy version of Blum-Blum-Shub algorithm that returns all bits)
- Coveyou generators: coveyou32, coveyou64, coveyou128.
- ICG (Inversive congruential generator): icg31x2, icg64, icg64_p2, hicg64.
- Based on Klimov-Shamir T-function: tf0_32, tf0_64, tf0_64, tf0_128. Some
  scrambled versions resembling PCG are also included.
- LCG-like with XOR instead of addition: pqrng32, pqrng64, pqrng128 by
  Karl-Uwe Frank. Actually based on some invertible an ergodic mappings
  obtained by V.S. Anashin.

The tf0 and pqrng generators are not widely known; they use some nonlinear
T-functions that have proven full periods.

pqrng T-function:

\f[
x_n = \left(x_{n-1} \oplus r\right) p \mod 2^{k}
\f]

tf0 T-function (also known as Klimov-Shamir "crazy" T-functon)

\f[
x_n = x_{n-1} + \left(x_{n-1}^2 \lor C\right) \mod 2^{k}
\f]

These T-functions are vulnerable to the `bspace4_8d_dec` test, just as LCGs
with power of 2 modulo, but perform better in other modifications of birthday
spacings and collision over tests.

## Some information about algorithms

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
