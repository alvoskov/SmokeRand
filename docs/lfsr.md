# LFSR period analyzer

The LFSR period analyzer allows to check if the LFSR period is maximal (i.e.
`2**n - 1`) or not. It also can deduce characteristic and jump polynomials
of the generator. It works directly with a compiled PRNG version and restores
its transition matrix by manipulating its internal state.

The key features of the analyzer:

- The PRNG must be compiled as a SmokeRand plugin or directly passed to
  SmokeRand functions, i.e. no `stdin`/`stdout` pipes are supported.
  A transtion matrix reconstruction requires a direct manipulation with
  PRNGs states anyway.
- The analyzer mathematically proves that the LFSR period is `2**n - 1`,
  not empirically checks it.
- Automatic deduction of the transition matrix, i.e. compiled PNG code
  is essentially used as a mathematical formula. So it allows to overcome
  problems with errors of conversion from "clever formula" to the program
  code.
- The deduction subroutine is fairly "foolproof" and contains some checks
  to prevent segfaults in the case of PRNGs with constants, pointers,
  file descriptors etc. in their states.
- 32, 48, 64, 96, 128, 160, 192, 256, 320, 512, 800, 1024 and 1600 bits
  of state are supported.

The analyzer consists of the next files:

- `src/lfsr_period.c` and `src/lfsr_period.h`: the analyzer core.
- `apps/test_lfsr_period.c`: some tests, basic sanity checks.
- `apps/find_xorshift_params.c`: a multithreaded program for LFSR shifts
  search and preliminary selection. Reproduces shifts triples for `xorshift32`
  and `xorshift64` obtained [by G. Marsaglia]((https://doi.org/10.18637/jss.v008.i14).

## Period verification algorithm

As it was mentioned above the verification of the period is based on
a mathematical proof, not on an empirical test. The verifier assumes that
the PRNG state is just a bit vector without any circle buffers, pointer
etc. The key steps are:

1. Check if the PNG has counters and/or constants (probable pointers or file
   descriptors). If it does have such constants then the automatic verification
   is impossible.
2. Restore the LFSR transition matrix over GF(2) field by setting its state
   to the `[10000...00]`, `[01000...00]`, `[00100...00]` etc. basis vectors
   with subsequent calls of the pseudorandom number generation subroutine.
3. Check if the PRNG is a really LFSR by calculating the \f$ A^{65537} \f$
   matrix and its comparison to the same matrix restored directly from the
   decimated PRNG output.
4. Check the \f$ A^{m} = I \f$ condition when \f$ m \f$ is the PRNG period.
5. Check the \f$ A^{\frac{m}{p_i}} \neq I \f$ conditions where \f$ p_i \f$
   are prime divisors of \f$ m \f$.


Note: since SmokeRand 0.50 steps 4-5 use an optimization based on
characteristic and jump polynomials instead of an explicit matrix
multiplication. It greatly improves performance for large PRNGs, i.e. that
have state sizes larger than 256 bits.

## About jump polynomials

The jump polynomials for LFSRs were described in the next research paper:

- Haramoto H., Matsumoto M., Nishimura T., Panneton F., L'Ecuyer P. Efficient
  Jump Ahead for F2-Linear Random Number Generators // INFORMS J. on Computing.
  2008. V. 20. N 3. P. 385–390. https://doi.org/10.1287/ijoc.1070.0251

Characteristic polynomials are converted to jump polynomials using
the following formula:

\f[
j(x) = x^n \mod p(x)
\f]

where \f$ n \f$ is the jump length. They are applied as follows:

\f[
x_{n} = b_0 x_0 + b_1 x_1 + \ldots b_{n-1} x_{n-1}
\f]

where \f$x_i\f$ are PRNG states and `+` is the bitwise XOR. That formula
may look a little bit mysterous but they are fairly easy to understand
if you remember that characteristic polynomials can be interpreted as
a recurrent formula for any particular bit of LFSR (e.g. xorshift) state:

\f[
c_0 b_0 \oplus c_1 b_1 \oplus \ldots \oplus c_n b_n = 0
\f]

Let's consider a simple example with \f$n = 5\f$ and the next characteristic
polynomial \f$ p(x) \f$:

\f[
p(x) = x^2 + x + 1 = 0 \Rightarrow x^2 = x + 1
\f]

Note that in the GF(2) field `+` and `-` are `^` (XOR), multiplication is
`&` (AND). 

\f[
\begin{array}{l}
x^3 = x \cdot x^2 = x(x + 1) = x^2 + x = x + x + 1 = 1 \\
x^4 = x \cdot x^3 = x \\
x^5 = x \cdot x^4 = x^2 = x + 1
\end{array}
\f]

Let's show a connection with the modular division:

    x3 | x2 + x + 1
       |--------------
       | x + 1
    x3 + x2 + x
    -----------
         x2 + x
         x2 + x + 1
         ----------
                  1 <-- result


    x5 | x2 + x + 1
       |--------------
       | x3 + x2 + 1
    x5 + x4 + x3
    -----------
         x4 + x3
         x4 + x3 + x2
         ------------
                   x2
                   x2 + x + 1
                   ----------
                        x + 1 <-- result

Also remember about the next property used for a fast exponentiation:

\f[
ab \mod m = \left(\left(a \mod m\right)\left(b \mod m\right)\right) \mod m
\f]

## Usage

There are two ways of the LFSR analysis usage: call a specialized `lfsr`
battery from the command line or use SmokeRand as a library.

An example of `lfsr` battery usage:

    $ ./smokerand lfsr generators/shr3.so

Examples of direct calls of SmokeRand functions can be found in the
`apps/test_lfsr_period.c` and `apps/find_xorshift_params.c`.

## Testing

There are several methods of the LFSR analyzer testing implemented in SmokeRand.

- `apps/test_lfsr_period.c` - basic sanity checks (periods of some generators,
  comparison of characteristic and jump polynomials for `xoroshiro128++` with
  reference values).
- `apps/find_xorshift_params.c` - reproduction of `xorshift32`, `xorshift64`
  and `xorshift128` shifts triples obtained by G. Marsaglia.
- Apply the `lfsr` battery to `shr3`, `xorshift48w16pp`, `xsh`, `dandelion64`,
  `xorshift96`, `dandelion128`, `xoroshiro128pp`, `xorshift160`, `xorshift256`,
  `xoshiro256pp`, `xorrot320`, `xorshiro512pp`, `xorgens512`, `xorgens1024`:
  all of them should have a full period of `2**n - 1`.
- Apply the `lfsr` to three different variants of `xorrot32`: the default one
  (has a full period). The `--param=bad1` and `--param=bad2` don't have full
  period.
- Apply the `lfsr` battery to `splitmix` and `sfc64`: it should return an
  error.

## Some sources of reference data:

Characteristic polynomials [for xorshift+](https://github.com/jj1bdx/xorshiftplus/blob/master/full/xorshift64poly.txt)

    xorshift64: 0.13-17-43 x^64 + x^49 + x^48 + x^45 + x^44 + x^42 + x^41 + x^38 + x^37 + x^28 + x^27 + x^26 + x^25 + x^17 + x^16 + x^11 + x^6 + x^5 + 1   19

Characteristic polynomials [for xorshift192/256](https://github.com/funny-falcon/xorshift256and192/blob/master/full/256shift64/prim.txt)

    xorshift256: 256 + x^242 + x^241 + x^240 + x^239 + x^234 + x^233 + x^232 + x^231 + x^226 + x^225 + x^224 + x^223 + x^220 + x^218 + x^217 + x^215 + x^212 + x^210 + x^206 + x^205 + x^203 + x^200 + x^199 + x^198 + x^195 + x^194 + x^191 + x^190 + x^189 + x^188 + x^185 + x^184 + x^180 + x^178 + x^177 + x^170 + x^169 + x^167 + x^165 + x^162 + x^159 + x^156 + x^153 + x^151 + x^150 + x^148 + x^147 + x^145 + x^143 + x^137 + x^136 + x^135 + x^133 + x^132 + x^127 + x^126 + x^125 + x^124 + x^121 + x^120 + x^119 + x^117 + x^116 + x^115 + x^114 + x^113 + x^112 + x^107 + x^106 + x^105 + x^100 + x^99 + x^97 + x^96 + x^94 + x^92 + x^89 + x^88 + x^87 + x^85 + x^83 + x^76 + x^75 + x^73 + x^72 + x^70 + x^69 + x^66 + x^65 + x^62 + x^59 + x^55 + x^51 + x^50 + x^49 + x^48 + x^47 + x^45 + x^43 + x^42 + x^40 + x^39 + x^38 + x^37 + x^35 + x^34 + x^33 + x^31 + x^30 + x^29 + x^20 + x^18 + x^17 + x^16 + x^14 + x^8 + x^7 + x^4 + x^3 + 1

Sebastiano Vigna site with some charateristic and jump polynomials for
xorshift/xoroshiro/xoroshiro PRNG family:

- https://prng.di.unimi.it/
- https://prng.di.unimi.it/xorshift.php


## Reproduced results

The `apps/find_xorshift_params.c` program was used for an exaustive search of
shifts triples for `xorshift32`, `xorshift64` and `xorshift128` that provide
the maximal PRNGs periods, i.e. `2**32 - 1`, `2**64 - 1` and `2**128 - 1`
respectively.

For `xorshift32` and `xorshift64` all existing triples were already found and
published in the [classical work](https://doi.org/10.18637/jss.v008.i14) by
G. Marsaglia about xorshift generators. For `xorshift128` Marsaglia published
only some triples. So this paper can be used as a test data set.

### xorshift64

All 275 `[a b c]` shifts triples for `xorshift64` obtained by G. Marsaglia were
successfully reproduced. The program output is given below:

    [ 1  1 54] [ 1  1 55] [ 1  3 45] [ 1  7  9] [ 1  7 44] [ 1  7 46] [ 1  9 50] [ 1 11 35] [ 1 11 50]
    [ 1 13 45] [ 1 15  4] [ 1 15 63] [ 1 19  6] [ 1 19 16] [ 1 23 14] [ 1 23 29] [ 1 29 34] [ 1 35  5]
    [ 1 35 11] [ 1 35 34] [ 1 45 37] [ 1 51 13] [ 1 53  3] [ 1 59 14] [ 2 13 23] [ 2 31 51] [ 2 31 53]
    [ 2 43 27] [ 2 47 49] [ 3  1 11] [ 3  5 21] [ 3 13 59] [ 3 21 31] [ 3 25 20] [ 3 25 31] [ 3 25 56]
    [ 3 29 40] [ 3 29 47] [ 3 29 49] [ 3 35 14] [ 3 37 17] [ 3 43  4] [ 3 43  6] [ 3 43 11] [ 3 51 16]
    [ 3 53  7] [ 3 61 17] [ 3 61 26] [ 4  7 19] [ 4  9 13] [ 4 15 51] [ 4 15 53] [ 4 29 45] [ 4 29 49]
    [ 4 31 33] [ 4 35 15] [ 4 35 21] [ 4 37 11] [ 4 37 21] [ 4 41 19] [ 4 41 45] [ 4 43 21] [ 4 43 31]
    [ 4 53  7] [ 5  9 23] [ 5 11 54] [ 5 15 27] [ 5 17 11] [ 5 23 36] [ 5 33 29] [ 5 41 20] [ 5 45 16]
    [ 5 47 23] [ 5 53 20] [ 5 59 33] [ 5 59 35] [ 5 59 63] [ 6  1 17] [ 6  3 49] [ 6 17 47] [ 6 23 27]
    [ 6 27  7] [ 6 43 21] [ 6 49 29] [ 6 55 17] [ 7  5 41] [ 7  5 47] [ 7  5 55] [ 7  7 20] [ 7  9 38]
    [ 7 11 10] [ 7 11 35] [ 7 13 58] [ 7 19 17] [ 7 19 54] [ 7 23  8] [ 7 25 58] [ 7 27 59] [ 7 33  8]
    [ 7 41 40] [ 7 43 28] [ 7 51 24] [ 7 57 12] [ 8  5 59] [ 8  9 25] [ 8 13 25] [ 8 13 61] [ 8 15 21]
    [ 8 25 59] [ 8 29 19] [ 8 31 17] [ 8 37 21] [ 8 51 21] [ 9  1 27] [ 9  5 36] [ 9  5 43] [ 9  7 18]
    [ 9 19 18] [ 9 21 11] [ 9 21 20] [ 9 21 40] [ 9 23 57] [ 9 27 10] [ 9 29 12] [ 9 29 37] [ 9 37 31]
    [ 9 41 45] [10  7 33] [10 27 59] [10 53 13] [11  5 32] [11  5 34] [11  5 43] [11  5 45] [11  9 14]
    [11  9 34] [11 13 40] [11 15 37] [11 23 42] [11 23 56] [11 25 48] [11 27 26] [11 29 14] [11 31 18]
    [11 53 23] [12  1 31] [12  3 13] [12  3 49] [12  7 13] [12 11 47] [12 25 27] [12 39 49] [12 43 19]
    [13  3 40] [13  3 53] [13  7 17] [13  9 15] [13  9 50] [13 13 19] [13 17 43] [13 19 28] [13 19 47]
    [13 21 18] [13 21 49] [13 29 35] [13 35 30] [13 35 38] [13 47 23] [13 51 21] [14 13 17] [14 15 19]
    [14 23 33] [14 31 45] [14 47 15] [15  1 19] [15  5 37] [15 13 28] [15 13 52] [15 17 27] [15 19 63]
    [15 21 46] [15 23 23] [15 45 17] [15 47 16] [15 49 26] [16  5 17] [16  7 39] [16 11 19] [16 11 27]
    [16 13 55] [16 21 35] [16 25 43] [16 27 53] [16 47 17] [17 15 58] [17 23 29] [17 23 51] [17 23 52]
    [17 27 22] [17 45 22] [17 47 28] [17 47 29] [17 47 54] [18  1 25] [18  3 43] [18 19 19] [18 25 21]
    [18 41 23] [19  7 36] [19  7 55] [19 13 37] [19 15 46] [19 21 52] [19 25 20] [19 41 21] [19 43 27]
    [20  1 31] [20  5 29] [21  1 27] [21  9 29] [21 13 52] [21 15 28] [21 15 29] [21 17 24] [21 17 30]
    [21 17 48] [21 21 32] [21 21 34] [21 21 37] [21 21 38] [21 21 40] [21 21 41] [21 21 43] [21 41 23]
    [22  3 39] [23  9 38] [23  9 48] [23  9 57] [23 13 38] [23 13 58] [23 13 61] [23 17 25] [23 17 54]
    [23 17 56] [23 17 62] [23 41 34] [23 41 51] [24  9 35] [24 11 29] [24 25 25] [24 31 35] [25  7 46]
    [25  7 49] [25  9 39] [25 11 57] [25 13 29] [25 13 39] [25 13 62] [25 15 47] [25 21 44] [25 27 27]
    [25 27 53] [25 33 36] [25 39 54] [28  9 55] [28 11 53] [29 27 37] [31  1 51] [31 25 37] [31 27 35]
    [33 31 43] [33 31 55] [43 21 46] [49 15 61] [55  9 56]
    Total number of triples: 275

### xorshift32

All 81 `[a b c]` shifts triples for `xorshift32` obtained by G. Marsaglia were
successfully reproduced. The program output is given below:

    [ 1  3 10] [ 1  5 16] [ 1  5 19] [ 1  9 29] [ 1 11  6] [ 1 11 16] [ 1 19  3] [ 1 21 20] [ 1 27 27]
    [ 2  5 15] [ 2  5 21] [ 2  7  7] [ 2  7  9] [ 2  7 25] [ 2  9 15] [ 2 15 17] [ 2 15 25] [ 2 21  9]
    [ 3  1 14] [ 3  3 26] [ 3  3 28] [ 3  3 29] [ 3  5 20] [ 3  5 22] [ 3  5 25] [ 3  7 29] [ 3 13  7]
    [ 3 23 25] [ 3 25 24] [ 3 27 11] [ 4  3 17] [ 4  3 27] [ 4  5 15] [ 5  3 21] [ 5  7 22] [ 5  9  7]
    [ 5  9 28] [ 5  9 31] [ 5 13  6] [ 5 15 17] [ 5 17 13] [ 5 21 12] [ 5 27  8] [ 5 27 21] [ 5 27 25]
    [ 5 27 28] [ 6  1 11] [ 6  3 17] [ 6 17  9] [ 6 21  7] [ 6 21 13] [ 7  1  9] [ 7  1 18] [ 7  1 25]
    [ 7 13 25] [ 7 17 21] [ 7 25 12] [ 7 25 20] [ 8  7 23] [ 8  9 23] [ 9  5 14] [ 9  5 25] [ 9 11 19]
    [ 9 21 16] [10  9 21] [10  9 25] [11  7 12] [11  7 16] [11 17 13] [11 21 13] [12  9 23] [13  3 17]
    [13  3 27] [13  5 19] [13 17 15] [14  1 15] [14 13 15] [15  1 29] [17 15 20] [17 15 23] [17 15 26]
    Total number of triples: 81

### xorshift128

Triples for `xorshift128` (32-bit version):

    [ 1  3 12] [ 1  3 15] [ 2  1 21] [ 2 21  6] [ 2 25  2] [ 3  2 21] [ 4  1  5] [ 5 12 29] [ 5 14  1]
    [ 6  5 17] [ 6 11 21] [ 6 11 25] [ 7 10  7] [ 7 11 19] [ 7 11 20] [ 7 12 11] [ 8 11 14] [ 9 11  6]
    [ 9 13 17] [ 9 24  1] [10  5  8] [10 11 12] [10 11 23] [11  5 12] [11  5 24] [11  5 26] [11  8 19]
    [11 10 21] [11 16  1] [13  3 25] [14  3 23] [14 13 19] [15  4 21] [17  7 21] [18 13 19] [19  1  2]
    [20  5 17] [21  2 23] [21  9  4] [21 16 11] [22  3 12] [23  3  6] [23 24  3] [25  3 10] [27  5 31]
    [27 19  5] [29  3 30]
    Total number of triples: 47

The `[5, 14, 1]`, `[15, 4, 21]`, `[23, 24, 3]`, `[5, 12, 29]` and `[11, 8, 19]`
triples were mentioned in the Marsaglia's article about xorshift PRNG family.

## New results

### xorrot

`xorrot` PRNG family was developed by A.L. Voskov, it resembles `xorshift` and
`xoroshiro`.

Triples for `xorrot160` (32-bit version):

    [ 3 19 31] [ 5  5 15] [ 7 24 27] [13  4 11]
    [13 11 23] [15  3 25] [17 17 27] [17 19 31]
    [19  2 15] [19  6 31] [23  9 24] [25 11 16]
    [29 23 29]
    Total number of triples: 13


Triples for `xorrot320` (64-bit version):

    [ 1 25 55] [ 1 41 63] [ 3 13 19] [ 3 15 30]
    [ 5 37 45] [ 7  9 60] [ 7 35 60] [ 9  2 25]
    [ 9  6 43] [ 9 17 26] [11  8 33] [11  9 43]
    [13 13 47] [13 25 38] [15 35 51] [17 25 39]
    [17 28 59] [21  9 49] [21 25 58] [25 12 29]
    [27  1 12] [27 22 49] [27 37 62] [27 43 61]
    [31 17 42] [31 24 29] [33 11 52] [37  8 39]
    [37 23 48] [37 55 56] [39  5 23] [39  6  7]
    [43  1 12] [43  1 28] [43  7 32] [43  9 14]
    [43 21 32] [47 35 47] [47 39 53] [47 47 59]
    [49 13 14] [49 37 48] [51  1 31] [51  5 38]
    [53  3 14] [53 13 37] [53 21 38] [55 33 48]
    [55 48 61] [55 49 56] [55 50 51] [57  5 20]
    [57 11 21] [57 22 39] [59  7 36] [59 11 39]

    Total number of triples: 56

Triples for `xorrot512` (64-bit version):

    [ 7  9 42]:0.996 [ 7 26 41]:0.275 [ 9  1 24]:0.684  [ 9  8 17]:0.609
    [ 9 25 60]:0.122 [11  4 13]:0.483 [11 35 40]:0.0791 [13  1 30]:0.758
    [13 26 33]:0.512 [15 19 49]:0.816 [15 25 55]:0.19   [15 27 46]:0.219
    [19 21 59]:0.039 [21 47 60]:0.323 [23  6 13]:0.49   [23 32 39]:0.861
    [27 13 16]:0.294 [29  3 50]:0.743 [29 23 52]:0.27   [31  3 61]:0.125
    [31 47 58]:0.147 [33  2  9]:0.794 [33 24 63]:0.316  [35 24 47]:0.197
    [35 25 28]:0.592 [37 29 36]:0.331 [39  7 52]:0.996  [39 33 48]:0.265
    [41  9 50]:0.705 [41 15 59]:0.199 [43 10 27]:0.23   [45 10 61]:0.279
    [49 10 57]:0.14  [51  1 14]:0.169 [51  4 55]:0.635  [51  8 25]:0.815
    [51 27 56]:0.629 [51 31 38]:0.59  [51 47 56]:0.35   [53  9 60]:0.04
    [55 29 34]:0.669 [57 14 21]:0.185 [57 29 32]:0.959  [57 34 41]:0.306
    [59 16 29]:0.526 [59 28 49]:0.192 [61 14 25]:0.389  [63 22 57]:0.866
    [63 54 57]:0.218

    Total number of triples: 49

### xoroshiro-w16

`xoroshiro48w16` and `xoroshiro64w16` are 16-bit xoroshiro modifications
developed by A.L. Voskov for retrocomputing and microcontrollers. The main
intention was to make 16-bit friendly but fairly robust generators.

The next settings for the `hamming_distr` test were used for triples screening:

    static const HammingDistrOptions
        hw_distr_sm = {.nvalues = 1ull << 28, .nlevels = 10};

Triples for `xoroshiro48w16`:

    [ 2  3  7]:0         [ 2  7  7]:1.66e-128 [ 2  9  9]:0 [ 2 11  7]:0
    [ 3  6  1]:1.26e-97  [ 3  8  9]:5.2e-169  [ 5  5  7]:0 [ 5 11  3]:8.58e-307
    [ 6  3  9]:0         [ 7  6  2]:2.06e-112 [ 7  7  5]:0 [ 9  3 10]:0
    [ 9  5 13]:6.55e-103 [ 9  9  9]:0         [10 13  1]:0 [11 10 12]:5.41e-251
    [11 11  2]:0         [13  1  8]:0         [13  1 13]:0 [13  6  7]:3.37e-123
    [13  8  7]:7.49e-194 [13 10  7]:3.41e-256 [15  1 14]:0 [15  5 13]:4.19e-108
    [15  7  4]:2.13e-137 [15 11  5]:2.08e-298 [15 11  7]:0

    Total number of triples: 27

Triples for `xoroshiro64w16`:

    [ 1  2 10]:0.00426  [ 1  6 14]:1.45e-19  [ 2 12  9]:3.53e-88 [ 3  4  8]:0.000453
    [ 3  8  8]:4.25e-43 [ 3 14  8]:1.66e-119 [ 4  5 13]:1.3e-14  [ 4  7  7]:5.77e-20
    [ 4 11 11]:1.93e-67 [ 4 15 13]:3.18e-131 [ 6  3  9]:8.35e-10 [ 7  7  2]:0
    [ 7 11  8]:3.87e-66 [ 7 15  8]:1.5e-131  [ 8 11  7]:0        [ 9  7  2]:1.61e-28
    [ 9 11 12]:3.29e-79 [10  1  7]:0.00119   [10  1 11]:0.311    [10  4  5]:5.95e-08
    [10  7 15]:2.68e-35 [11  3 10]:9.41e-25  [11 14  8]:1.5e-126 [13  8  8]:2.05e-40
    [14 11 11]:1.29e-80 [15  6  6]:2.38e-22

    Total number of triples: 26

## Interesting notes by G. Jones

G. Jones (also known as D. Blackman), one of the developers of the
`xoroshiro`/`xoshiro` PRNG families, made the next post that is very useful
for understanding of the LFSR underlying mathematical principles:

    Re: characteristic polynomials for F2 linear prngs.

    The characteristic polynomial corresponds to a linear feadback shift register
    prng which xors together some previous bits at fixed lags to make
    a new bit.

    Take (almost any?) F2 linear prng and look at one bit per result
    always at the same position. The bitstream is the one you would get from the lfsr.
    Look at the stream from another bit position, and it's the same characteristic
    polynomial, but with a different starting state.

    From the obvious counting argument, many different F2 linear prngs share the
    same characteristic polynomial. If the polynomial is a good one, some of these
    prngs will be good, and some will be bad. THe polynomial can tell you some things
    about the prng, but not everything, because the prng has more things in it
    than just the polynomial.

    Working with the polynomial can be faster than working with the matrix for
    stuff like checking that the prng is full cycle, or calculating the table of
    mysterious constants for a jumpahead function. For something as big as mt19936
    characteristic polynomial is really the only way to do these things. For
    256 bits you could do it all with matrixes if you hate characteristic
    polynomials enough, or just can't get them to work.

    If you want a rough heuristic on whether F2 linear prng will be any good,
    the matrix offers better guidance than the characteristic polynomial.
    Check the number of 1 bits in each row and column. If the least is
    is at least 9 or so, that's kind of ok, but if you can achieve better
    without hurting performance too much, go for it. The ideal is close
    to 50% in each row, and in each column. For large matrixes, you could probably
    get away with a lot less. Also generate the inverse matrix and run
    the same check.

    BTW the practice of calling the shift registers ""polynomials"" comes from
    algebra of finite fields. You can add and multiply them according to those
    rules. And yes, you can use the polynomial as a function. The input is
    just one bit, and so is the rather boring output. This is part of the
    definition, but rarely something you would want to do.

The original post can be found
[here](https://sourceforge.net/p/pracrand/discussion/366935/thread/416f25e937/)

