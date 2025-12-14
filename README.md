# SmokeRand

SmokeRand is a set of tests for pseudorandom number generators (PRNGs). Tested
generators should return either 32-bit or 64-bit unsigned uniformly distributed
unsigned integers.

The current version of SmokeRand can be found [here](https://github.com/alvoskov/SmokeRand).

## Key features

- Direct support of both 32-bit and 64-bit generators without taking
  upper/lower parts, interleaving etc. Even the lowest bits are tests.
- Two interfaces for PRNGs: either `stdin`/`stdout` or C API for plugins
  loaded as shared libraries.
- A minimalistic but rather sensitive and fast set of statistical tests.
  They are grouped in several batteries that take from 1 second (`express`)
  to less than 1 hour (`full`).
- A simple heuristic scoring of generators. 
- Cross-platform multithreading support (only for PRNGs implemented as
  shared libraries).
- Easy integration with TestU01 and PractRand.

## Quick start

SmokeRand is written in C99. Compile SmokeRand using GNU Make (`Makefile.gnu`
or `make_pkg.sh` if DEB package is desired) or CMake (`CMakeLists.txt`), see
[Compilation](#compilation) section for details. Then test one of the supplied
generators implemented as plugins:

```sh
$ make -f Makefile.gnu
$ cd bin
$ ./smokerand default generators/lcg64.so [--threads]
```

You can also write a program that sends your PRNG output as binary stream to
`stdout` and connect it to SmokeRand through a pipe:

```sh
$ py ../misc/pyrand.py | ./smokerand default stdin32
```

An example of Python script that will send an infinite stream of bytes to stdout:

```python
import sys, random
while True:
    random_bytes = random.randbytes(1024)
    sys.stdout.buffer.write(random_bytes)
```

A similar example in C99 for a 64-bit linear congruential generator:

```c
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#if defined(_MSC_VER) || defined(__WATCOMC__)
#include <io.h>
#endif
#define BUFSIZE 256
int main() {
    uint64_t x = (uint64_t) time(NULL);
    uint32_t buf[BUFSIZE];
#if defined(_WIN32)
    (void) _setmode( _fileno(stdout), _O_BINARY);
#endif
    while (1) {
        for (size_t i = 0; i < BUFSIZE; i++) {
            x = 6906969069U * x + 12345U;
            buf[i] = x >> 32;
        }
        fwrite(buf, sizeof(uint32_t), BUFSIZE, stdout);
    }
    return 0;
}
```

See details in the [How to test a PRNG](#how-to-test-a-prng) section.

## Compilation

SmokeRand supports four software build systems: GNU Make, CMake, Ninja and
WMake (Open Watcom make). All plugins with pseudorandom number generators
are built as dynamic libraries: `.dll` (dynamic linked libraries) for Windows
and 32-bit DOS and `.so` (shared objects) for GNU/Linux and other UNIX-like
systems. Usage of GNU Make and CMake is considered here, information about
Ninja, WMake, compilation for DOS and other technical details can be found in
[Technical notes](docs/technotes.md)

### GNU Make

The manually written script for GNU Make is `Makefile.gnu` and doesn't require
CMake. GNU Make: supports gcc, clang (as zig cc) but not MSVC or Open Watcom.
Has `install` and `uninstall` pseudotargets for GNU/Linux.

If you work under Debian-based GNU/Linux distribution - you can run the
`make_pkg.sh` script that will compile SmokeRand and will create `.deb` package.
Otherwise just use GNU Make (`install` is optional):

```sh
$ make -f Makefile.gnu
$ make -f Makefile.gnu install
```

This makefile supports GNU/Linux, Windows (MinGW) and DOS (DJGPP).

### CMake

```sh
$ cmake CMakeLists.txt [-G "Target_platform"]
$ make
```


## Batteries

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

## How to test a PRNG

The are two ways to test PRNGs using SmokeRand:

1. Through stdin/stdio pipes. Simple but doesn't support multithreading.
2. Load PRNG from a shared library. More complex but allows multithreading.
   A lot of PRNG plugins are supplied with SmokeRand.

The first method can be used to test e.g. PRNG from Python standard library:

    $ py ../misc/pyrand.py | ./smokerand default stdin32

Use `stdin32` for 32-bit generators and `stdin64` for 64-bit generators
respectively.

The second method is to test the generator implemented as a plugin. A lot of
plugins are supplied with SmokeRand, the source code is in the `generators/`
directory. Compiled plugins are inside the `bin/generators/` directory.
Use the next command to test the generator:

    $ ./smokerand default generators/lcg64.so [--threads]

### Integration with PractRand

SmokeRand can send the generator output to `stdout` in binary form and this
can be used for testing the PRNG with PractRand:

    $ ./smokerand stdout generators/lcg64.so | RNG_test stdin32 -multithreaded

Getting PRNG input from stdin (multithreading is not supported in this case):

    $ ./smokerand stdout generators/lcg64.so | ./smokerand express stdin32

### Integration with TestU01

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

## More details

The more detailed information about SmokeRand tests, supplied generators,
batteries and other technical details can be found at:

- [Generators](docs/generators.md): a short reviews of supplied PRNGs.
- [Plugins](docs/plugins.md): description of C API for plugins with PRNGs.
- [Results](docs/results.md): statistical tests results.
- [Technical notes](docs/technotes.md): details about compilation, heuristic
  scaling, built-in and custom batteries etc.
- [Tests](docs/tests.md): a detailed description of statistical tests.
- [Existing solutions](docs/review): a review of other existing PRNG test
  suites (TestU01, PractRand etc.).
