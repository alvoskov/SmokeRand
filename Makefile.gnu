#
# Classic makefile for SmokeRand.
#

# For gcc
CC = gcc
AR = ar
GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
# For zig cc
#CC = zig cc
#AR = zig ar
#GEN_CFLAGS = -fPIC
# compiling flags here
# -Werror 
PLATFORM_FLAGS=-DNO_X86_EXTENSIONS -DNOTHREADS
IS_PORTABLE=0
#PLATFORM_FLAGS = -m32 -DNO_X86_EXTENSIONS
#IS_PORTABLE=1
#PLATFORM_FLAGS=-m32
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
LIB_SOURCES = $(addprefix $(SRCDIR)/, core.c coretests.c entropy.c extratests.c fileio.c lineardep.c)
LIB_HEADERS = $(addprefix $(INCLUDEDIR)/, cinterface.h core.h coretests.h entropy.h extratests.h fileio.h lineardep.h)
LIB_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(LIB_SOURCES)))
# Battery
BAT_SOURCES = $(addprefix $(SRCDIR)/, bat_antilcg.c bat_brief.c bat_default.c bat_full.c)
BAT_HEADERS = $(addprefix $(INCLUDEDIR)/, bat_antilcg.h bat_brief.h bat_default.h bat_full.h)
BAT_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(BAT_SOURCES)))
# Executables
EXE_NAMES = smokerand calibrate_dc6 test_funcs
EXE_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXE_NAMES)))

# Generators
ifeq ($(IS_PORTABLE), 1)
GEN_SOURCES = $(addsuffix _shared.c,$(addprefix generators/, alfib_mod alfib \
    chacha coveyou64 crand drand48 isaac64 kiss64 kiss93 kiss99 lcg32prime \
    lcg64 lcg69069 lfib_par lfsr113 lfsr258 loop_7fff_w64 minstd mlfib17_5 \
    msws mt19937 mulberry32 mwc64x mwc64 pcg32 pcg64 philox32 r1279 \
    randu ranluxpp rc4 romutrio rrmxmx sfc32 sfc64 shr3 speck128 \
    splitmix32 splitmix sqxor32 swblux swbw swb threefry tinymt32 tinymt64 \
    well1024a xoroshiro1024stst xoroshiro1024st xoroshiro128pp xoroshiro128p \
    xorshift128p xorshift128 xorwow xsh))
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

$(GEN_OBJFILES): $(BINDIR)/generators/obj/%.o : generators/%.c $(INCLUDEDIR)/cinterface.h $(INCLUDEDIR)/core.h
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_shared.o : generators/superduper64_shared.c \
    generators/superduper64_body.h $(INCLUDEDIR)/cinterface.h $(INCLUDEDIR)/core.h
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_u32_shared.o : generators/superduper64_u32_shared.c \
    generators/superduper64_body.h $(INCLUDEDIR)/cinterface.h $(INCLUDEDIR)/core.h
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
