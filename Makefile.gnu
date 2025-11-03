#
# Classic makefile for SmokeRand.
# Supports GCC and Clang (as Zig CC) compilers.
#
# Usage:
# make -f Makefile.gnu
# make -f Makefile.gnu PLATFORM_NAME=GCC
#
# Supported platforms: GCC, GCC32, DJGPP, MINGW-HX, ZIGCC
# Note: GCC includes MinGW.
#
# DJGPP platform compiles all generators as DXE3 modules using the dxe3gen
# experimental linker supplied with DJGPP.
#

PLATFORM_NAME ?= GCC
LIB_SOURCES_EXTRA =
LIB_HEADERS_EXTRA =
GEN_LFLAGS = 
GEN_DISABLED =
ifeq ($(PLATFORM_NAME), GCC)
    CC = gcc
    CXX = g++
    AR = ar
    GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
    PLATFORM_FLAGS = -g -march=native
else ifeq ($(PLATFORM_NAME), GCC32)
    CC = gcc
    CXX = g++
    AR = ar
    GEN_CFLAGS = -fPIC  -DNO_CUSTOM_DLLENTRY
    PLATFORM_FLAGS = -m32 -march=native
else ifeq ($(PLATFORM_NAME), DJGPP)
    CC = gcc
    CXX = gpp
    AR = ar
    GEN_CFLAGS = -ffreestanding -nostdlib
    # NOTE: threefry xxtea may be refactored!
    GEN_DISABLED = icg64 mrg32k3a sezgin63 threefry wich1982 wich2006 xxtea
    PLATFORM_FLAGS = -m32 -march=i586 -DNOTHREADS -U__STRICT_ANSI__
    LIB_SOURCES_EXTRA = pe32loader.c
    LIB_HEADERS_EXTRA = pe32loader.h
else ifeq ($(PLATFORM_NAME), MINGW-HX)
    CC = gcc
    CXX = g++
    AR = ar
    GEN_CFLAGS = -fPIC -DNO_CUSTOM_DLLENTRY -DUSE_WINTHREADS
    PLATFORM_FLAGS = -m32 -march=i686
else ifeq ($(PLATFORM_NAME), ZIGCC)
    CC = zig cc
    CXX = zic c++
    AR = zig ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS = -DUSE_WINTHREADS -march=native
else ifeq ($(PLATFORM_NAME), GENERIC)
    CC = gcc
    CXX = g++
    AR = ar
    GEN_CFLAGS = -fPIC
    PLATFORM_FLAGS = -DNO_X86_EXTENSIONS -DNOTHREADS -DNO_CUSTOM_DLLENTRY
endif
#-----------------------------------------------------------------------------
CFLAGS = $(PLATFORM_FLAGS) -std=c99 -O3 -Werror -Wall -Wextra -Wstrict-aliasing
CXXFLAGS = $(PLATFORM_FLAGS) -std=c++11 -O3 -Werror -Wall -Wextra -Wstrict-aliasing
CFLAGS89 = $(PLATFORM_FLAGS) -std=c89 -O3 -Werror -Wall -Wextra -Wstrict-aliasing
LINKFLAGS = $(PLATFORM_FLAGS)
INCLUDE = -Iinclude

APPSRCDIR = apps
SRCDIR = src
OBJDIR = obj
BINDIR = bin
LIBDIR = lib
INCLUDEDIR = include/smokerand
LFLAGS =  -L$(LIBDIR) -lsmokerand_bat -lsmokerand_core -lm
ifeq ($(OS), Windows_NT)
#GEN_LFLAGS = 
#-Wl,--exclude-all-symbols
EXE = .exe
SO = .dll
else
#GEN_LFLAGS =
EXE =
SO = .so
ifeq ($(PREFIX),)
PREFIX = /usr/local
endif
endif

# DJGPP specific extensions: DXE3 modules and .exe for executables
ifeq ($(PLATFORM_NAME), DJGPP)
EXE = .exe
SO = .dxe
endif

# Core library
CORE_LIB = $(LIBDIR)/libsmokerand_core.a
LIB_SOURCES = $(addprefix $(SRCDIR)/, $(LIB_SOURCES_EXTRA) core.c coretests.c \
    blake2s.c entropy.c extratests.c fileio.c lineardep.c hwtests.c specfuncs.c threads_intf.c)
LIB_HEADERS = $(addprefix $(INCLUDEDIR)/, $(LIB_HEADERS_EXTRA) apidefs.h cinterface.h core.h coretests.h \
    blake2s.h entropy.h extratests.h fileio.h lineardep.h hwtests.h int128defs.h specfuncs.h threads_intf.h x86exts.h) \
    include/smokerand_core.h
LIB_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(LIB_SOURCES)))
INTERFACE_HEADERS = $(INCLUDEDIR)/apidefs.h $(INCLUDEDIR)/cinterface.h \
    $(INCLUDEDIR)/int128defs.h $(INCLUDEDIR)/x86exts.h
# Battery
BAT_LIB = $(LIBDIR)/libsmokerand_bat.a
BATLIB_SOURCES = $(addprefix $(SRCDIR)/, bat_express.c bat_brief.c bat_default.c bat_file.c bat_full.c bat_special.c)
BATLIB_HEADERS = $(addprefix $(INCLUDEDIR)/, bat_express.h bat_brief.h bat_default.h bat_file.h bat_full.h bat_special.h) \
    include/smokerand_bat.h
BATLIB_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(BATLIB_SOURCES)))
# Executables
EXEC_NAMES = smokerand sr_tiny calibrate_linearcomp calibrate_dc6 \
    test_crand test_funcs test_rdseed
EXEC_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXEC_NAMES)))
EXECXX_NAMES = test_cpp11
EXECXX_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXECXX_NAMES)))

# Generators
GEN_CUSTOM_SOURCES = $(addsuffix .c,$(addprefix generators/, ranluxpp))
GEN_DISABLED_SOURCES = $(addsuffix .c,$(addprefix generators/, $(GEN_DISABLED)))
GEN_ALL_SOURCES = $(filter-out $(GEN_DISABLED_SOURCES), $(wildcard generators/*.c))
GEN_SOURCES = $(filter-out $(GEN_CUSTOM_SOURCES), $(GEN_ALL_SOURCES))
GEN_OBJFILES = $(patsubst %.c,%.o,$(subst generators/,$(BINDIR)/generators/obj/,$(GEN_SOURCES)))
GEN_SHARED = $(patsubst %.c,%$(SO),$(subst generators/,$(BINDIR)/generators/, $(GEN_ALL_SOURCES)))
GEN_BINDIR = $(BINDIR)/generators

#-----------------------------------------------------------------------------
all: $(CORE_LIB) $(BAT_LIB) $(addprefix $(BINDIR)/, $(addsuffix $(EXE),$(EXEC_NAMES))) \
    $(addprefix $(BINDIR)/, $(addsuffix $(EXE),$(EXECXX_NAMES))) generators

$(CORE_LIB): $(LIB_OBJFILES)
	$(AR) rcu $@ $^

$(BAT_LIB): $(BATLIB_OBJFILES)
	$(AR) rcu $@ $^

$(BINDIR)/sr_tiny$(EXE): $(APPSRCDIR)/sr_tiny.c $(SRCDIR)/specfuncs.c include/smokerand/specfuncs.h
	$(CC) $(CFLAGS89) $(LINKFLAGS) $(APPSRCDIR)/sr_tiny.c $(SRCDIR)/specfuncs.c -o $@ -lm $(INCLUDE) 

$(BINDIR)/smokerand$(EXE): $(OBJDIR)/smokerand.o $(CORE_LIB) $(BAT_LIB) $(BAT_HEADERS)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(BINDIR)/calibrate_dc6$(EXE): $(OBJDIR)/calibrate_dc6.o $(CORE_LIB) $(BAT_LIB)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(BINDIR)/calibrate_linearcomp$(EXE): $(OBJDIR)/calibrate_linearcomp.o $(CORE_LIB)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(BINDIR)/test_cpp11$(EXE): $(OBJDIR)/test_cpp11.o $(CORE_LIB) $(BAT_LIB)
	$(CXX) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(BINDIR)/test_crand$(EXE): $(OBJDIR)/test_crand.o $(CORE_LIB) $(BAT_LIB)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)
                          
$(BINDIR)/test_funcs$(EXE): $(OBJDIR)/test_funcs.o $(CORE_LIB) $(BAT_LIB)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(BINDIR)/test_rdseed$(EXE): $(OBJDIR)/test_rdseed.o $(CORE_LIB) $(BAT_LIB)
	$(CC) $(LINKFLAGS) $< -o $@ $(LFLAGS) $(INCLUDE)

$(LIB_OBJFILES) $(BATLIB_OBJFILES): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(EXEC_OBJFILES): $(OBJDIR)/%.o : $(APPSRCDIR)/%.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(EXECXX_OBJFILES): $(OBJDIR)/%.o : $(APPSRCDIR)/%.cpp $(LIB_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@


.PHONY: clean generators install uninstall

generators: $(GEN_SHARED)

# Generic rules for linking PRNG plugins
ifeq ($(PLATFORM_NAME), DJGPP)
$(GEN_BINDIR)/%$(SO): $(GEN_BINDIR)/obj/%.o
	dxe3gen $< -o $@
else
$(GEN_BINDIR)/%$(SO): $(GEN_BINDIR)/obj/%.o
	$(CC) $(LINKFLAGS) -shared $(GEN_CFLAGS) $< -s $(GEN_LFLAGS) -o $@
endif

$(GEN_OBJFILES): $(BINDIR)/generators/obj/%.o : generators/%.c $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

# Generators that depend on some extra header files that should be taken
# into account in their dependency rules.
$(GEN_BINDIR)/obj/ranluxpp.o : generators/ranluxpp.c \
    generators/ranluxpp_helpers.h generators/ranluxpp_mulmod.h $(INTERFACE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) $(GEN_CFLAGS) -c $< -o $@

clean:
ifeq ($(OS), Windows_NT)
	del $(BINDIR)\smokerand.exe
	del $(BINDIR)\calibrate_dc6.exe
	del $(BINDIR)\calibrate_linearcomp.exe
	del $(BINDIR)\sr_tiny.exe
	del $(BINDIR)\test_cpp11.exe
	del $(BINDIR)\test_funcs.exe
	del $(BINDIR)\test_rdseed.exe
	del $(OBJDIR)\*.o /q
	del $(LIBDIR)\*.a /q
	del $(BINDIR)\generators\*.dll /q
	del $(BINDIR)\generators\obj\*.o
	del $(BINDIR)\generators\*.a /q
else
	rm -f $(BINDIR)/smokerand
	rm -f $(BINDIR)/calibrate_dc6
	rm -f $(BINDIR)/calibrate_linearcomp
	rm -f $(BINDIR)/sr_tiny
	rm -f $(BINDIR)/test_funcs
	rm -f $(BINDIR)/test_cpp11
	rm -f $(BINDIR)/test_rdseed
	rm -f $(OBJDIR)/*.o
	rm -f $(LIBDIR)/*
	rm -f $(BINDIR)/generators/*.so
	rm -f $(BINDIR)/generators/obj/*.o
endif

install:
ifeq ($(OS), Windows_NT)
	echo `make install` is supported only for UNIX-like systems.
else
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/man
	install -m 755 $(BINDIR)/smokerand $(DESTDIR)$(PREFIX)/bin
	install -m 644 $(LIBDIR)/libsmokerand_bat.a $(DESTDIR)$(PREFIX)/lib
	install -m 644 $(LIBDIR)/libsmokerand_core.a $(DESTDIR)$(PREFIX)/lib
	gzip -c smokerand.1 > smokerand.1.gz
	install -d $(DESTDIR)$(PREFIX)/man/man1
	install -m 644 smokerand.1.gz $(DESTDIR)$(PREFIX)/man/man1
	install -d $(DESTDIR)$(PREFIX)/include/smokerand
	install -m 644 include/smokerand/*.h $(DESTDIR)$(PREFIX)/include/smokerand
	install -m 644 include/smokerand_bat.h $(DESTDIR)$(PREFIX)/include
	install -m 644 include/smokerand_core.h $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/lib/smokerand/generators
	install -m 644 $(GEN_BINDIR)/*.so $(DESTDIR)$(PREFIX)/lib/smokerand/generators

	install -d $(DESTDIR)$(PREFIX)/src/smokerand
	install -d $(DESTDIR)$(PREFIX)/src/smokerand/generators
	install -d $(DESTDIR)$(PREFIX)/src/smokerand/src
	install -d $(DESTDIR)$(PREFIX)/src/smokerand/apps
	install -m 644 src/*.c $(DESTDIR)$(PREFIX)/src/smokerand/src
	install -m 644 apps/*.c $(DESTDIR)$(PREFIX)/src/smokerand/apps
	install -m 644 apps/*.asm $(DESTDIR)$(PREFIX)/src/smokerand/apps
	install -m 644 Makefile.gnu $(DESTDIR)$(PREFIX)/src/smokerand
	install -m 644 CMakeLists.txt $(DESTDIR)$(PREFIX)/src/smokerand
	install -m 644 generators/*.c $(DESTDIR)$(PREFIX)/src/smokerand/generators
	install -m 644 generators/*.h $(DESTDIR)$(PREFIX)/src/smokerand/generators

	install -d $(DESTDIR)$(PREFIX)/etc/smokerand
	install -m 644 batscripts/*.cfg $(DESTDIR)$(PREFIX)/etc/smokerand
endif

uninstall:
ifeq ($(OS), Windows_NT)
	echo `make uninstall` is supported only for UNIX-like systems.
else
	rm -f  $(DESTDIR)$(PREFIX)/bin/smokerand
	rm -f  $(DESTDIR)$(PREFIX)/lib/libsmokerand_bat.a
	rm -f  $(DESTDIR)$(PREFIX)/lib/libsmokerand_core.a
	rm -f  $(DESTDIR)$(PREFIX)/man/man1/smokerand.1.gz
	rm -f  $(DESTDIR)$(PREFIX)/include/smokerand/*
	rm -f  $(DESTDIR)$(PREFIX)/include/smokerand_bat.h
	rm -f  $(DESTDIR)$(PREFIX)/include/smokerand_core.h
	rm -fd $(DESTDIR)$(PREFIX)/include/smokerand
	rm -f  $(DESTDIR)$(PREFIX)/lib/smokerand/generators/*.so
	rm -fd $(DESTDIR)$(PREFIX)/lib/smokerand/generators
	rm -fd $(DESTDIR)$(PREFIX)/lib/smokerand

	rm -f  $(DESTDIR)$(PREFIX)/src/smokerand/generators/*
	rm -fd $(DESTDIR)$(PREFIX)/src/smokerand/generators
	rm -f  $(DESTDIR)$(PREFIX)/src/smokerand/src/*
	rm -fd $(DESTDIR)$(PREFIX)/src/smokerand/src
	rm -f  $(DESTDIR)$(PREFIX)/src/smokerand/apps/*
	rm -fd $(DESTDIR)$(PREFIX)/src/smokerand/apps
	rm -f  $(DESTDIR)$(PREFIX)/src/smokerand/*
	rm -fd $(DESTDIR)$(PREFIX)/src/smokerand

	rm -f  $(DESTDIR)$(PREFIX)/etc/smokerand/*.cfg
	rm -fd $(DESTDIR)$(PREFIX)/etc/smokerand
endif
