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

The next compilers are supported: GCC (including MinGW and DJGPP), Clang (as
zig cc), MSVC (Microsoft Visual C) and Open Watcom C. It allows to compile
SmokeRand under Windows, UNIX-like systems and DOS. A slightly modified 16-bit
version (`apps/sr_tiny.c`) of `express` battery can be compiled even by Borland
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

Notes about DJGPP:

- All plugins with PRNGs are compiled as DXE3 modules using the `dxe3gen`
  linker that is specific to DJGPP. It is possible to make SmokeRand DJGPP
  build to use DLLs but it will require a cross-compilation of plugins with
  MinGW or Open Watcom that is not too practical.
- SmokeRand man page can be converted to the overstrike based format using
  the next commands:

    $ groff -mandoc -Tascii -P-cBoU smokerand.1 > smokerand.dos
    $ less smokerand.dos

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
"next bit test", i.e. based on cryptographical primitives such as block or
stream ciphers.

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


# Usage

To test the generator the next command can be used:

    $ ./smokerand default generators/lcg64.so --threads

Integration with PractRand:

    $ ./smokerand stdout generators/lcg64.so | RNG_test stdin32 -multithreaded

Getting PRNG input from stdin (multithreading is not supported in this case):

    $ ./smokerand stdout generators/lcg64.so | ./smokerand express stdin32

Integeration with TestU01 using the TestU01-threads plugin:

    $ ./smokerand s=libtestu01th_sr_ext.so generators/lcg64.so --batparam=SmallCrush --threads

For 64-bit generators you can test them in different modes:

- Interleaved 32-bit parts: `--filter=interleaved32`
- Higher 32 bits (default): `--filter=high32`
- Lower 32 bits: `--filter=low32`

The plugin can be downloaded from https://github.com/alvoskov/TestU01-threads


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
 brief   | 25              | 2^35                | 2^36
 default | 42              | 2^37                | 2^38
 full    | 46              | 2^40                | 2^41


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
