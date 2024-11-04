#
# Classic makefile for SmokeRand.
#
CC = gcc
# compiling flags here
CFLAGS = -std=c99 -O2 -Wall -Werror -Wextra -Wno-attributes -march=native
GEN_CFLAGS = -fPIC -ffreestanding -nostdlib
INCLUDE = -Iinclude

SRCDIR = src
OBJDIR = obj
BINDIR = bin
LIBDIR = lib
INCLUDEDIR = include/smokerand
LFLAGS =  -L$(LIBDIR) -lsmokerand_core -lm
ifeq ($(OS), Windows_NT)
GEN_LFLAGS = -Wl,--exclude-all-symbols
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
BAT_SOURCES = $(addprefix $(SRCDIR)/, bat_brief.c bat_default.c bat_full.c)
BAT_HEADERS = $(addprefix $(INCLUDEDIR)/, bat_brief.h bat_default.h bat_full.h)
BAT_OBJFILES = $(subst $(SRCDIR),$(OBJDIR),$(patsubst %.c,%.o,$(BAT_SOURCES)))
# Executables
EXE_NAMES = smokerand calibrate_dc6 test_funcs
EXE_OBJFILES = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(EXE_NAMES)))

# Generators
GEN_SOURCES = $(wildcard generators/*.c)
GEN_OBJFILES = $(patsubst %.c,%.o,$(subst generators/,$(BINDIR)/generators/obj/,$(GEN_SOURCES)))
GEN_SHARED = $(patsubst %.c,%$(SO),$(subst generators/,$(BINDIR)/generators/lib,$(GEN_SOURCES)))
GEN_BINDIR = $(BINDIR)/generators

all: $(CORE_LIB) $(addprefix $(BINDIR)/, $(addsuffix $(EXE),$(EXE_NAMES))) generators

$(CORE_LIB): $(LIB_OBJFILES)
	ar rcu $@ $^

$(BINDIR)/smokerand$(EXE): $(OBJDIR)/smokerand.o $(CORE_LIB) $(BAT_OBJFILES) $(BAT_HEADERS)
	$(CC) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(BINDIR)/calibrate_dc6$(EXE): $(OBJDIR)/calibrate_dc6.o $(CORE_LIB)
	$(CC) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(BINDIR)/test_funcs$(EXE): $(OBJDIR)/test_funcs.o $(CORE_LIB)
	$(CC) $< $(BAT_OBJFILES) -o $@ $(LFLAGS) $(INCLUDE) 

$(LIB_OBJFILES) $(BAT_OBJFILES) $(EXE_OBJFILES): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: clean generators

generators: $(GEN_SHARED)

$(GEN_BINDIR)/libcrand_shared$(SO): $(GEN_BINDIR)/obj/crand_shared.o
	$(CC) -shared $< -s $(GEN_LFLAGS) -o $@

$(GEN_BINDIR)/lib%$(SO): $(GEN_BINDIR)/obj/%.o
	$(CC) -shared $(GEN_CFLAGS) $< -s $(GEN_LFLAGS) -o $@

$(GEN_OBJFILES): $(BINDIR)/generators/obj/%.o : generators/%.c $(INCLUDEDIR)/cinterface.h
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
