# Compilation

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

32-bit DOS version uses a simplified custom DLL loader (see the `src/pe32loader.c`
and `include/pe32loader.h` files) that allows to use properly compiled plugins
with PRNGs even without DOS extender with PE files support. 16-bit DOS
version (see `apps/sr_tiny.c`) doesn't support dynamic libraries at all,
tested generators should be embedded into its source code.
