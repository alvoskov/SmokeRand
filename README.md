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

# Compilation

SmokeRand supports four software build systems: GNU Make, CMake, Ninja and
WMake (Open Watcom make). All plugins with pseudorandom number generators
are built as dynamic libraries: `.dll` (dynamic linked libraries) for Windows
and 32-bit DOS and `.so` (shared objects) for GNU/Linux and other UNIX-like systems.
Usage of GNU Make and CMake is considered here, information about Ninja, WMake,
compilation for DOS and other technical details can be found in
[Technical notes](docs/technotes.md)

## GNU Make

The manually written script for GNU Make is `Makefile.gnu` and doesn't require
CMake. GNU Make: supports gcc, clang (as zig cc) but not MSVC or Open Watcom.
Has `install` and `uninstall` pseudotargets for GNU/Linux.

If you work under Debian-based GNU/Linux distribution - you can run the
`make_pkg.sh` script that will compile SmokeRand and will create `.deb` package.
Otherwise just use GNU Make (`install` is optional):

    $ make -f Makefile.gnu
    $ make -f Makefile.gnu install

## CMake

    $ cmake CMakeLists.txt
    $ make

# How to test a pseudorandom number generator

The are two ways to test PRNGs using SmokeRand:

1. Through stdin/stdio pipes. Simple but doesn't support multithreading.
2. Load PRNG from a shared library. More complex but allows multithreading.
   A lot of PRNG plugins are supplied with SmokeRand.

To test the generator the next command can be used:

    $ ./smokerand default generators/lcg64.so --threads

## Integration with PractRand

SmokeRand can send the generator output to `stdout` in binary form and this
can be used for testing the PRNG with PractRand:

    $ ./smokerand stdout generators/lcg64.so | RNG_test stdin32 -multithreaded

Getting PRNG input from stdin (multithreading is not supported in this case):

    $ ./smokerand stdout generators/lcg64.so | ./smokerand express stdin32

## Integration with TestU01

TestU01 is a well known and very respected set of statistical tests for
pseudorandom number generators. It can be used as a custom battery for
SmokeRand by means of the [TestU01-threads](https://github.com/alvoskov/TestU01-threads)
wrapper. This wrapper supports parallel run of tests from SmallCrush, Crush and
BigCrush batteries by implementing its own multithreading dispatcher.

    $ ./smokerand s=libtestu01th_sr_ext.so generators/lcg64.so --batparam=SmallCrush --threads

SmokeRand resolves an old problem of 64-bit PRNG testing by supplying different
filters than transform 64-bit PRNG into a 32-bit one:

- Interleaved 32-bit parts: `--filter=interleaved32`
- Higher 32 bits (default): `--filter=high32`
- Lower 32 bits: `--filter=low32`

# Batteries

Four batteries are implemented in SmokeRand:

- `express` - simplified battery that consists of 7 tests and can be
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

Custom batteries of tests also can be created as both scripts and dynamic
libraries.

# More details

The more detailed information about SmokeRand tests, supplied generators,
batteries and other technical details can be found at:

- [Generators](docs/generators.md)
- [Plugins](docs/plugins.md)
- [Results](docs/results.md)
- [Technical notes](docs/technotes.md)
- [Tests](docs/tests.md)

# Existing solutions

Existing solutions:

1. [TestU01](https://doi.org/10.1145/3447773). Has a very comprehensive set of
   tests and an excellent documentation. Doesn't support 64-bit generator,
   has issues with analysis of lowest bits.
2. [PractRand](https://pracrand.sourceforge.net/). Very limited number of tests
   but they are good ones and detect flaws in a lot of PRNGs. Tests are mostly
   suggested by the PractRand author and is not described in scientific
   publications. Their advantage -- they accumulate information from a sequence
   of 1024-byte blocks and can be applied to arbitrary large samples.
3. [Ent](https://www.fourmilab.ch/random/). Only very basic tests, even 64-bit
   LCG will probably pass them.
4. [Dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php).
   Resembles DIEHARD, but contains more tests and uses much larger samples.
   Less sensitive than TestU01.
5. [gjrand](https://gjrand.sourceforge.net/). Has some unique and sensitive
   tests but documentation is scarce.
