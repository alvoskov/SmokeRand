#
# Classic makefile for SmokeRand.
# Supports GCC and Clang (as Zig CC) compilers.
#

PLATFORM_NAME=MINGW-HX
ifeq ($(PLATFORM_NAME), GCC)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
    PLATFORM_FLAGS = -march=native
    IS_PORTABLE = 0
else ifeq ($(PLATFORM_NAME), GCC32)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC  -DNO_CUSTOM_DLLENTRY
    PLATFORM_FLAGS = -m32 -march=native
    IS_PORTABLE = 1
else ifeq ($(PLATFORM_NAME), MINGW-HX)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC -DNO_CUSTOM_DLLENTRY
    PLATFORM_FLAGS = -m32 -march=i686
    IS_PORTABLE = 1
else ifeq ($(PLATFORM_NAME), ZIGCC)
    CC = zig cc
    AR = zig ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS = -DUSE_WINTHREADS -march=native
    IS_PORTABLE = 0
else ifeq ($(PLATFORM_NAME), GENERIC)
    CC = gcc
    AR = ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS = -DNO_X86_EXTENSIONS -DNOTHREADS -DNO_CUSTOM_DLLENTRY
    IS_PORTABLE = 1
endif
#-----------------------------------------------------------------------------
CFLAGS = $(PLATFORM_FLAGS) -std=c99 -O3 -Werror -Wall -Wextra -Wno-attributes
CFLAGS89 = $(PLATFORM_FLAGS) -std=c89 -O3 -Werror -Wall -Wextra -Wno-attributes -march=native
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
LIB_SOURCES = $(addprefix $(SRCDIR)/, core.c coretests.c \
    entropy.c extratests.c fileio.c lineardep.c hwtests.c specfuncs.c)
LIB_HEADERS = $(addprefix $(INCLUDEDIR)/, apidefs.h cinterface.h core.h coretests.h \
    entropy.h extratests.h fileio.h lineardep.h hwtests.h specfuncs.h)
LIB_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(LIB_SOURCES)))
INTERFACE_HEADERS = $(INCLUDEDIR)/apidefs.h $(INCLUDEDIR)/cinterface.h $(INCLUDEDIR)/core.h
# Battery
BAT_SOURCES = $(addprefix $(SRCDIR)/, bat_express.c bat_brief.c bat_default.c bat_file.c bat_full.c)
BAT_HEADERS = $(addprefix $(INCLUDEDIR)/, bat_express.h bat_brief.h bat_default.h bat_file.h bat_full.h)
BAT_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(BAT_SOURCES)))
# Executables
EXE_NAMES = smokerand sr_tiny calibrate_dc6 test_funcs
EXE_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXE_NAMES)))

# Generators
GEN_CUSTOM_SOURCES = $(addsuffix _shared.c,$(addprefix generators/, ranluxpp \
    superduper64 superduper64_u32))
ifeq ($(IS_PORTABLE), 1)
GEN_ALL_SOURCES = $(addsuffix _shared.c,$(addprefix generators/, \
    alfib_lux alfib_mod alfib ara32 chacha cmwc4096 coveyou64 crand \
    cwg64 des drand48 efiix64x48 flea32x1 hc256 isaac64 kiss64 kiss93 \
    kiss99 lcg128_u32_portable lcg32prime lcg64 lcg69069 lcg96_portable \
    lfib4 lfib4_u64 lfib_par lfsr113 lfsr258 loop_7fff_w64 lrnd64 \
    lxm_64x128 macmarsa magma minstd mixmax mlfib17_5 mrg32k3a msws_ctr \
    msws mt19937 mulberry32 mwc1616x mwc1616 mwc3232x mwc32x mwc4691 \
    mwc64x mwc64 pcg32 pcg32_streams pcg32_xsl_rr pcg64_64 philox2x32 \
    philox32 r1279 randu ranlux48 ranluxpp ranq1 ranq2 ranrot32 ranrot_bi \
    ranshi ranval ran rc4ok rc4 romutrio rrmxmx sapparot2 sapparot \
    sezgin63 sfc16 sfc32 sfc64 sfc8 shr3 speck128 splitmix32 splitmix \
    sqxor32 stormdrop_old stormdrop superduper64 superduper64_u32 \
    superduper73 swblarge swblux swbw swb taus88 threefry2x64 threefry \
    tinymt32 tinymt64 well1024a xorgens xoroshiro1024stst xoroshiro1024st \
    xoroshiro128pp xoroshiro128p xorshift128p xorshift128 xorwow \
    xsh ))
else
GEN_ALL_SOURCES = $(wildcard generators/*.c)
endif
GEN_SOURCES = $(filter-out $(GEN_CUSTOM_SOURCES), $(GEN_ALL_SOURCES))
GEN_OBJFILES = $(patsubst %.c,%.o,$(subst generators/,$(BINDIR)/generators/obj/,$(GEN_SOURCES)))
GEN_SHARED = $(patsubst %.c,%$(SO),$(subst generators/,$(BINDIR)/generators/lib, $(GEN_ALL_SOURCES)))
GEN_BINDIR = $(BINDIR)/generators

#-----------------------------------------------------------------------------
all: $(CORE_LIB) $(addprefix $(BINDIR)/, $(addsuffix $(EXE),$(EXE_NAMES))) generators

$(CORE_LIB): $(LIB_OBJFILES)
	$(AR) rcu $@ $^

$(BINDIR)/sr_tiny$(EXE): $(SRCDIR)/sr_tiny.c $(SRCDIR)/specfuncs.c include/smokerand/specfuncs.h
	$(CC) $(CFLAGS89) $(LINKFLAGS) $(SRCDIR)/sr_tiny.c $(SRCDIR)/specfuncs.c -o $@ -lm $(INCLUDE) 

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

# Linking crand PRNG requires linking with C standard library
$(GEN_BINDIR)/libcrand_shared$(SO): $(GEN_BINDIR)/obj/crand_shared.o
	$(CC) $(LINKFLAGS) -shared $< -s $(GEN_LFLAGS) -o $@

# Generic rules for linking PRNG plugins
$(GEN_BINDIR)/lib%$(SO): $(GEN_BINDIR)/obj/%.o
	$(CC) $(LINKFLAGS) -shared $(GEN_CFLAGS) $< -s $(GEN_LFLAGS) -o $@

$(GEN_OBJFILES): $(BINDIR)/generators/obj/%.o : generators/%.c $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

# Generators that depend on some extra header files that should be taken
# into account in their dependency rules.
$(GEN_BINDIR)/obj/ranluxpp_shared.o : generators/ranluxpp_shared.c \
    generators/ranluxpp_helpers.h generators/ranluxpp_mulmod.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_shared.o : generators/superduper64_shared.c \
    generators/superduper64_body.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

$(GEN_BINDIR)/obj/superduper64_u32_shared.o : generators/superduper64_u32_shared.c \
    generators/superduper64_body.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

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
