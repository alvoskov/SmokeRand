# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)

## Unreleased

## [0.45] - 2026-01-10

### Added

- `arxfw8`, `arxfw8ex`, `arxfw8ex2`, `arxfw8ex3`, `arxfw16`, `arxfw16ex2`, `arxfw32`,
  `arxfw64` nonlinear generators with linear parts.
- `biski32` and `biski64` modifications with improved shifts (cease to fail Hamming
  weights based test).
- `mrc16`, `mrc32`, `mrc64` generators.
- `xkiss32_rot_aox`, `xoroshiro64aox`, `xoshiro128aox` generators that don't
  use integers overflows; may be easy to port to Modula-2 and Oberon programming
  languages.
- `xorrot32` generator: designed as a part of `xkiss32_rot_aox` generator.
- `prvhash16cw` algorithm.
- `rdrand` algorithm (direct access to the x86 RDRAND, based on the code
  by Scott Miller)
- Internal self-tests for `gjrand8` and `gjrand16`.
- `-D__USE_MINGW_ANSI_STDIO` flag for older versions of MinGW.
- Preliminary aarch64 support.
- `stdoutfl` and `stdoutflx` outputs: double precision numbers output in the
  human-readable format. May be useful for e.g. exporting of cryptographic 
  generators output to Python, Octave etc.
- `--version` command line key.

### Changed

- Improved initialization procedures for `a5rand`, `komirand`, `prvhash` PRNGs
  families (recommended by their author, A.Vaneev)
- Fixed `-Wconversion` warnings for GCC 6-9.
- `pthread_t` is now not assumed to be integer (was bad for `pthread-win32`)
- Bugfix: `inttypes.h` is excluded from PRNG plugins (may not present in
  freestanding environments)
- Bugfix: improved protection from infinite cycles in the gap test. See
  `loop_antigap_w64` and `loop_7fff_w64` generators: they are test that are
  designed to provoke such infinite loops.
- Custom DLL entry for MSYS added into `apidefs.h`.


## [0.44] - 2025-12-15

### Added

- `a5rand`, `a5rand32`, `a5rand32w`, `a5randw` algorithms.
- `komirand`, `komirand16`, `komirand16w`, `komirand32`, `komirand32w`,
  `komirandw` algorithms.
- `prvhash12c`, `prvhash12cw`, `prvhash16c`, `prvhash64c`, `prchash64cw` algorithms.
- `xoshiro256p`, `xoshiro256pp`, `xoshiro256stst` algorithms.

### Changed

- [Changelog](CHANGELOG.md) is in the separate file now.
- [Pseudorandom number generators tests results](docs/results.md) is in the
  separate file now.
- [Readme](README.md) is separated into several files, they are in the `docs/`
  directory now.
- Bugfix with GCC 14-15 warnings about mismatched `calloc` arguments.
- Bugfix in `src/sr_tiny.c`: xorshift32 parameters: now they are (13,17,5)
  instead of (17,13,5).
- `undef` were added to battery files to simplify unity builds of `smokerand`
  executable.
- CMakeLists.txt cleanup.

## [0.43] - 2025-12-07

### Added

- Algorithms `threefish1024`, `mwc63x2`, `xtea2`, `xtea2_64`.
- Experimental nonlinear generators `rge256lite`, `rge256ex`, `rge256exctr`,
  `rge512ex`, `rge512exctr`.
- Custom batteries implemented as shared libraries are supported now, see
  the `apps/bat_example.c` and `include/smokerand/plugindefs.h` files.
  It allows to use TestU01 as a plugin (see the
  https://github.com/alvoskov/TestU01-threads repository with such plugin
  containing a multi-threaded version of TestU01)
- New command line key `--testname=name`. Now it is possible to
  write e.g. `--testname=gap16_count0` instead of manual search of numeric
  testid for the `--testid=id` key.

### Changed

- Bug was fixed in the `gap16_count0` test: occasional false failures for
  cryptographic algorithms (wrong computation of `[value .. 0 .. value]`
  gaps percentage was fixed). The bug was likely introduced in v0.41 after
  refactoring after enabling of `-Wconversion` flag.

## [0.42] - 2025-11-16

### Added

- The first public release.
- ChaCha8 added to the `chacha` PRNG. Also AVX2 version of ChaCha now returns
  the same sequence as the portable version.
- `ranrot8tiny`, `ranrot16tiny`, `ranrot32tiny`, `ranrot64tiny` custom PRNGs.

### Changed

- Some code cleanup.

## [0.41] - 2025-11-10

### Added

- The `--seed` command line key was added, it allows to use two kinds of seeds
  for the internal seeder based on Blake2s and ChaCha20. Their usage allows to
  fully reproduce any test battery run for deterministic PRNG.
- Tests dispatcher for multithreaded mode made fully deterministic that made
  fully reproducible runs possible.
- License texts for Blake2s and MIXMAX were added.
- Some scripts for PRNG self-tests and `express` battery runs were added.

### Changed

- Improvements and bugfixes in initialization procedures of several PRNGs.
- Code cleanup and some bugfixes after `-Wconversion` and `-Wpedantic` flags.


## [0.40] - 2025-10-25

### Added

- DJGPP preliminary support; plugins with pseudorandom number generators are
  compiled as DXE3 modules using the `dxe3gen` linker from DJGPP.
- Custom entropy collector based on RDTSC and the system PIC for such platforms
  as Open Watcom DOS32 and DJGPP.

### Changed

- Some bugfixes and code cleanup.


## [0.39] - 2025-10-15

### Added

- `mt19937_64`: a 64-bit version of Mersenne Twister, two modifications.
- Custom `jctr32` and `jctr64` generators based on experimental block ciphers
  by Bob Jenkins. These block ciphers were turned to the construction
  resembling the ChaCha20 stream cipher.

### Changed

- `mt19937`: code cleanup and additon of a test vector.


## [0.38] - 2025-09-28

### Added

- New seeds generator, now it is based on ChaCha20 and uses `/dev/urandom`
  or MS Windows built-in CSPRNG for seeding (if available).
- New generators were added: `blabla` (experimental 64-bit version of `chacha`),
  `lcg42` and some other generators.

### Changed

- Some bugfixes.


## [0.37] - 2025-06-28

### Added

- The `gap_inv8` test was added to the `brief`, `default` and `full` batteries,
  it is required to catch the `swbmwc32` generator and probably other scrambled
  subtract-with-borrow generators. The test is not very sensitive but may also
  detect additive/subtractive lagged Fibonacci generators with small states.
- Some new generators were added: e.g. `combo`, `ncombo`, `mzran13`,
  `mtc8/16/32/64`, `kiss96` (and other KISSes), `swbmwc32`, `swbmwc64`, reduced
  versions of `gjrand` etc.


## [0.36] - 2025-06-15

### Added

- Several new xoroshiro generators: `xoroshiro32pp`, `xoroshiro64pp`,
  `xoroshiro64*`, `xoroshiro64**` etc.
- Updated `biski64` generator, `biski64_alt` modification that passes the
  `hamming_distr` test (pairwise xoring).
- Several new KISS family generators including the custom `kiss64_awc`
  generator that doesn't contain multiplications.
- Several new MWC and SWB generator including the `cswb4288` generator
  by G. Marsaglia; the `cswb4288` PRNG is interesting for `gap16_count0`
  test checking (it fails this test). The `cmwc4827` also fails this test.
- New custom SMWC (scrambled MWC) family: experimental.
- `hamming_distr` test: bugfix, now it checks if PRNG output is wider
  than declared. In earlier versions it causes segfaults.
- `ranval64` chaotic generator by Bob Jenkins also known as RKISS.


## [0.35] - 2025-05-02

### Added

- Several new generators: `biski64`, `melg19937`, `mwc8222`, `xabc`,
  Wichmann-Hill (1982,2006), several inverse congruential generators.


## [0.34] - 2025-05-01

### Added

- Several new generators.
- The new `hamming_distr` test that is based on calculation of n-bit blocks
  Hamming weights and analysis of their distribution. Also processes pairwise
  XORs of that blocks. It was added to the `brief`, `default` and `full`
  batteries.


## [0.33] - 2025-04-10

### Added

- A portable implementation of 128-bit arithmetics subset was added to the
  header files (see `include/smokerand/int128defs.h`). It is designed mainly
  for 128-bit LCGs.
- A new nonlinear generator `wob2m` by Bob Jenkins.
- A portable software implementation of AES was added.

### Changed

- All generators are either portable or at least have portable versions.


## [0.32] - 2025-03-28

### Added

- The variants of the `xxtea` algorithm with 128-bit and 256-bit blocks
  were added (selected by means of the `--param=mode` command line option).
  Options: `scalar-128`, `avx2-128`, `scalar-256`, `avx2-256`.


## [0.31] - 2025-03-26

### Added

- The `mwc32xxa8`, `mwc40xxa8`, `mwc48xxa16`, `mwc128xxa32`, `mwc256xxa16`,
  `threefry_avx`, `xtea` and `xtea_avx` generators were added.

### Changed

- `threefry` and `threefry_avx` generators now support `Threefry` and
  `Threefish` modes as `--param=mode`.
- Improved debug output in the `gap` and `gap16_count0` tests.


## [0.30] - 2025-03-18

### Added

- `apps/test_crand.c`: an example of PRNG testing using static linking with
  SmokeRand core.

### Changed

- `brief` battery extended with the `mod3` test. It is fast but catches some
  nonlinear generators such as `ara32` and `flea32x1`.
- `radixsort64`: switches now to `quicksort64` if there is not enough memory
  for the internal buffers.
- 64-bit birthday paradox uses the custom `quicksort64` instead of `qsort`:
  a slight improvement in performance was achieved.
- Entropy module ror64 bugfix.

### Removed

- `testgens` removed because 32-bit DOS versions of SmokeRand supports DLLs now.
  And for other platforms without shared library support the examples from
  `apps/test_crand.c` and `apps/test_cpp11.cpp` are much more useful.


## [0.29] - 2025-03-16

### Added

- DLL support for 32-bit DOS version was added by means of implementation of
  a simplified PE32 loader.

### Changed

- `express` battery extended: `bspace4_8d` and `bspace8_4d` tests were added.
  It allows to detect flaws in `xorwow` and `mwc1616` generators respectively.
  The `sr_tiny` executable (C89 implementation of `express` battery) was also
  upgraded.
- Improved makefiles for Open Watcom C: freestanding DLLs, 32-bit and 16-bit
  DOS versions.
- Improvements and simplification in a multithreaded battery launcher: dynamic
  assignment of tests to threads, randomization. Estimated times and RAM
  usage intensity were removed from tests info: they were not relevant and
  accurate anyway.
- Some bugfixes in makefiles, especially for zig cc (clang). Now freestanding
  libraries linking is also supported for zig cc / clang.


## [0.28] - 2025-03-10

### Added

- `gap16_count0`: "not enough memory" messages added.
- `Makefile.d32` file for building a simplified version of SmokeRand 
  for 32-bit DOS extenders using Open Watcom C.
- `testgens`: a simplified version of SmokeRand for platforms without DLL
  support. Mainly a hack for DOS version.
- `xoshiro128+` and `xoshiro128++` generators were added.
- Internal self-tests for some LFSR generators were added.


## [0.27] - 2025-03-07

### Changed

- Improved computation of p-values for the linear complexity test.
- Bugfix in the rrmxmx generator.
- Test vectors for some generators.
- Generators source files are renames (`_shared` suffix excluded).


## [0.26] - 2025-03-05

- Multithreading support was transferred to a separate file.
- Improved threads IDs printing.
- `make install`, `make uninstall` and deb package support
  for UNIX-like systems was added.


## [0.25] - 2025-03-04

### Added

- lrnd64 (a modification of lrnd32-1023) generator.
- Open Watcom C support for Windows NT target. It is always a 32-bit build
  and may have performance issues, especially with modern CSPRNGs. But it
  was successfully tested in FreeDOS (HX DOS Extender).

### Changed

- Extension of Lua scripts for Ninja build system.
- `freq` battery now shows bytes/words that caused failure.


## [0.24] - 2025-02-25

### Added

- man page draft added.
- lua script for build.ninja.
- A new `--report-brief` key for brief final reports.

### Changed

- `aesni` fixes for clang.
- PractRand results updated for some generators.


## [0.23] - 2025-02-12

### Added

- efiix64x48, DES-CTR, xorgens were added.

### Changed

- In `full` battery number of bits in linearcomp tests was increased to
  1 million, mainly due to experiments with lower bits of xorgens and
  other scrambled LFSRs.


## [0.22] - 2025-02-09

### Added

- LXM PRNG
- Scripted version of the `ising` battery.

### Changed

- Refactoring.


## [0.21] - 2025-02-08

- Scripted versions of all four general purpose batteries: `express`, `brief`,
  `default`, `full`.
- Refactoring of C versions of batteries.

## [0.20] - 2025-02-07

### Added

- Preliminary support of user-defined batteries stored in text files.
- Philox2x32 and 'squares' CBPRNG by B.Widynski.

### Changed

- Refactoring that allowed to get rid of a lot of small functions in the
  battery description (support of user-defined data for tests was added).


## [0.19] - 2025-01-02

### Added

- CSPRNG based on Magma GOST R 34.12-2015 block cipher (CTR and CBC modes).


## [0.18] - 2025-01-31

- segfault was fixed in the `gap16_count0` test and memory leaks --- in
  the `nbit_words_freq_test` test.
- Small refactoring of the `gap16_count0` test.
- AVX2 versions of xoroshiro128++ and xorshift128+ (new PRNG)
- Improved seed generator, uses more entropy sources and machine ID.


## [0.17] - 2025-01-29

- Birthday test now works with 32-bit generators.
- New PRNGs including Threefry2x64x20 (scalar and AVX2 versions).


## [0.16] - 2024-12-23

- Speck128/128 with reduced number of rounds.
- `dos16` renamed to `express`.
- Correct program termination in the case of PRNG example allocation failure.
  It may occur in the case of incorrect optional parameter.


## [0.15] - 2024-12-16

- RC4OK generator was added.
- Fine tuning of some `hamming_ot_low` tests in `full` battery: samples sizes
  are doubled.
- Added results of several tests.
- `-O3` key in `Makefile.gnu`.


## [0.14] - 2024-12-12

- AES-128 based PRNG added (only for x86-64 processors with AESNI support).
- Bugfix in linearcomp test (memory leak).
- An improved report in `sr_tiny.c` --- compact version of `dos16` battery for
  16-bit computers.


## [0.13] - 2024-12-09

### Changed

- Bugfix in `nbit_words_freq_test`: more accurate sample size. Also input
  arguments were packed into structure.
- `dos16` battery: samples sizes are changed to fit into ~64 MiB of data
  (16 MiB of data for hypothetical sample reusage between tests). Preparation
  to finishing its compact 16-bit version.


## [0.12] - 2024-12-09

### Added

- SumCollector test.
- Several new generators.

### Changed

- Bugfix in `nbit_words_freq_test`: in the case of 16-bit words the words
were overlapping (shift was fixed).


## [0.11] - 2024-12-02

### Added

- ranrot32 PRNG.
- Also a new modification of Hamming weights based test was added to `default`
  and `full` battery (uses 256-bit words, allows to find longer-range
  correlations and detect lagged Fibonacci generators and RANROT).


## [0.1] - 2024-12-01

An initial version.

Requires some testing, extension of documentation, completion of `dos16`
battery implementation for 16-bit computers. Tests lists for batteries are
frozen before 1.0 release.
