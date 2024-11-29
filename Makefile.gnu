#
# Classic makefile for SmokeRand.
#

PLATFORM_NAME=GCC
ifeq ($(PLATFORM_NAME), GCC)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
    PLATFORM_FLAGS=
    IS_PORTABLE=0
else ifeq ($(PLATFORM_NAME), GCC32)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
    PLATFORM_FLAGS=-m32
    IS_PORTABLE=0
else ifeq ($(PLATFORM_NAME), ZIGCC)
    CC = zig cc
    AR = zig ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS=-DNO_X86_EXTENSIONS -DUSE_WINTHREADS
    IS_PORTABLE=0
else ifeq ($(PLATFORM_NAME), GENERIC)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS=-DNO_X86_EXTENSIONS -DNOTHREADS
    IS_PORTABLE=1
endif
#------------------------------------------
CFLAGS = $(PLATFORM_FLAGS) -std=c99 -O2 -Werror -Wall -Wextra -Wno-attributes -march=native
LINKFLAGS = $(PLATFORM_FLAGS)
INCLUDE = -Iinclude

SRCDIR = src
OBJDIR = obj
BINDIR = bin
LIBDIR = lib
INCLUDEDIR = include/smokerand
LFLAGS =  -L$(LIBDIR) -lsmokerand_core -lm
ifeq ($(OS), Windows_NT)
GEN_LFLAGS = 
#-Wl,--exclude-all-symbols
EXE = .exe
SO = .dll
else
GEN_LFLAGS =
EXE =
SO = .so
endif


# Core library
CORE_LIB = $(LIBDIR)/libsmokerand_core.a
LIB_SOURCES = $(addprefix $(SRCDIR)/, core.c coretests.c entropy.c extratests.c fileio.c lineardep.c specfuncs.c)
LIB_HEADERS = $(addprefix $(INCLUDEDIR)/, apidefs.h cinterface.h core.h coretests.h entropy.h extratests.h fileio.h lineardep.h specfuncs.h)
LIB_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(LIB_SOURCES)))
INTERFACE_HEADERS = $(INCLUDEDIR)/apidefs.h $(INCLUDEDIR)/cinterface.h $(INCLUDEDIR)/core.h
# Battery
BAT_SOURCES = $(addprefix $(SRCDIR)/, bat_dos16.c bat_brief.c bat_default.c bat_full.c)
BAT_HEADERS = $(addprefix $(INCLUDEDIR)/, bat_dos16.h bat_brief.h bat_default.h bat_full.h)
BAT_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(BAT_SOURCES)))
# Executables
EXE_NAMES = smokerand calibrate_dc6 test_funcs
EXE_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXE_NAMES)))

# Generators
ifeq ($(IS_PORTABLE), 1)
GEN_SOURCES = $(addsuffix _shared.c,$(addprefix generators/, alfib_mod alfib \
    chacha coveyou64 crand drand48 isaac64 kiss64 kiss93 kiss99 lcg32prime \
    lcg64 lcg96_portable lcg128_u32_portable lcg69069 lfib_par lfsr113 lfsr258 \
    loop_7fff_w64 minstd mlfib17_5 msws mt19937 mulberry32 mwc64x mwc64 pcg32 \
    pcg64 philox32 r1279 randu ranluxpp rc4 romutrio rrmxmx sapparot sapparot2
    sfc8 sfc16 sfc32 sfc64 shr3 speck128 splitmix32 splitmix sqxor32 \
    superduper73 superduper64 superduper64_u32 swblux swbw swb threefry \
    tinymt32 tinymt64 well1024a xoroshiro1024stst xoroshiro1024st \
    xoroshiro128pp xoroshiro128p xorshift128p xorshift128 xorwow xsh))
else
GEN_SOURCES = $(wildcard generators/*.c)
endif
GEN_OBJFILES = $(patsubst %.c,%.o,$(subst generators/,$(BINDIR)/generators/obj/,$(GEN_SOURCES)))
GEN_SHARED = $(patsubst %.c,%$(SO),$(subst generators/,$(BINDIR)/generators/lib,$(GEN_SOURCES)))
GEN_BINDIR = $(BINDIR)/generators

all: $(CORE_LIB) $(addprefix $(BINDIR)/, $(addsuffix $(EXE),$(EXE_NAMES))) generators

$(CORE_LIB): $(LIB_OBJFILES)
	$(AR) rcu $@ $^

$(BINDIR)/smokerand$(EXE): $(OBJDIR)/smokerand.o $(CORE_LIB) $(BAT_OBJFILES) $(BAT_HEADERS)
	$(CC) $(LINKFLAGS) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(BINDIR)/calibrate_dc6$(EXE): $(OBJDIR)/calibrate_dc6.o $(CORE_LIB)
	$(CC) $(LINKFLAGS) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(BINDIR)/test_funcs$(EXE): $(OBJDIR)/test_funcs.o $(CORE_LIB)
	$(CC) $(LINKFLAGS) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(LIB_OBJFILES) $(BAT_OBJFILES) $(EXE_OBJFILES): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean generators

generators: $(GEN_SHARED)

$(GEN_BINDIR)/libcrand_shared$(SO): $(GEN_BINDIR)/obj/crand_shared.o
	$(CC) $(LINKFLAGS) -shared $< -s $(GEN_LFLAGS) -o $@

$(GEN_BINDIR)/lib%$(SO): $(GEN_BINDIR)/obj/%.o
	$(CC) $(LINKFLAGS) -shared $(GEN_CFLAGS) $< -s $(GEN_LFLAGS) -o $@

$(GEN_OBJFILES): $(BINDIR)/generators/obj/%.o : generators/%.c $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_shared.o : generators/superduper64_shared.c \
    generators/superduper64_body.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_u32_shared.o : generators/superduper64_u32_shared.c \
    generators/superduper64_body.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

# TODO: RANLUX!


clean:
ifeq ($(OS), Windows_NT)
	del $(BINDIR)\*.exe
	del $(OBJDIR)\*.o /q
	del $(LIBDIR)\*.a /q
	del $(BINDIR)\generators\*.dll /q
	del $(BINDIR)\generators\obj\*.o
	del $(BINDIR)\generators\*.a /q
else
	rm $(BINDIR)/smokerand
	rm $(BINDIR)/calibrate_dc6
	rm $(BINDIR)/test_funcs
	rm $(OBJDIR)/*.o
	rm $(LIBDIR)/*
	rm $(BINDIR)/generators/*.so
	rm $(BINDIR)/generators/obj/*.o
endif
