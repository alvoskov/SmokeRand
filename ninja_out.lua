--
-- Creates build.ninja file for SmokeRand.
--


local gen_sources = {'aesni', 'alfib_lux', 'alfib_mod', 'alfib', 'ara32',
    'chacha_avx',  'chacha', 'cmwc4096', 'coveyou64', 'crand', 'cwg64', 'des',
    'drand48', 'efiix64x48', 'flea32x1', 'hc256', 'isaac64', 'kiss64', 'kiss93',
    'kiss99', 'lcg128_full', 'lcg128', 'lcg128_u32_full_rot', 'lcg128_u32_full',
    'lcg128_u32_portable', 'lcg32prime', 'lcg64prime',  'lcg64', 'lcg69069',
    'lcg96_portable', 'lcg96', 'lfib4', 'lfib4_u64', 'lfib_par', 'lfsr113',
    'lfsr258', 'loop_7fff_w64', 'lxm_64x128', 'macmarsa', 'magma_avx', 'magma',
    'minstd', 'mixmax', 'mlfib17_5', 'mrg32k3a', 'msws_ctr', 'msws', 'mt19937',
    'mulberry32', 'mwc128x', 'mwc128', 'mwc1616x', 'mwc1616', 'mwc3232x', 'mwc32x',
    'mwc4691', 'mwc64x', 'mwc64', 'pcg32', 'pcg32_streams', 'pcg32_xsl_rr', 'pcg64_64',
    'pcg64_xsl_rr', 'philox2x32', 'philox32', 'philox', 'r1279', 'randu', 'ranlux48',
    'ranluxpp', 'ranq1', 'ranq2', 'ranrot32', 'ranrot_bi', 'ranshi', 'ranval', 'ran',
    'rc4ok', 'rc4', 'romutrio', 'rrmxmx', 'sapparot2', 'sapparot', 'sezgin63', 'sfc16',
    'sfc32', 'sfc64', 'sfc8', 'shr3', 'speck128_avx', 'speck128_r16_avx', 'speck128',
    'splitmix32', 'splitmix', 'sqxor32', 'sqxor', 'stormdrop_old', 'stormdrop',
    'superduper64', 'superduper64_u32', 'superduper73', 'swblarge', 'swblux',
    'swbw', 'swb', 'taus88', 'threefry2x64_avx', 'threefry2x64', 'threefry',
    'tinymt32', 'tinymt64', 'well1024a', 'wyrand', 'xorgens', 'xoroshiro1024stst',
    'xoroshiro1024st', 'xoroshiro128pp_avx', 'xoroshiro128pp', 'xoroshiro128p',
    'xorshift128pp_avx', 'xorshift128p', 'xorshift128', 'xorwow', 'xsh'}

local stub = [[cflags = -std=c99 -O3 -Werror -Wall -Wextra -Wno-attributes -march=native
cflags89 = -std=c89 -O3 -Werror -Wall -Wextra -Wno-attributes -march=native
gen_cflags = $cflags -fPIC -ffreestanding -nostdlib
cc = gcc
srcdir = src
objdir = obj
bindir = bin
libdir = lib
includedir = include/smokerand
gen_bindir = $bindir/generators
gen_srcdir = generators
exe_linkflags =
so_linkflags = -shared
gen_linkflags = -shared -fPIC -ffreestanding -nostdlib


rule cc
    command = $cc -MMD -MT $out -MF $out.d $cflags -c -Iinclude $in -o $out
    description = CC $out
    depfile = $out.d
    deps = gcc

rule cc89
    command = $cc -MMD -MT $out -MF $out.d $cflags89 -c -Iinclude $in -o $out
    description = CC $out
    depfile = $out.d
    deps = gcc


rule cc_gen
    command = $cc -MMD -MT $out -MF $out.d $gen_cflags -c -Iinclude $in -o $out
    description = CC $out
    depfile = $out.d
    deps = gcc

rule ar
    command = ar rcu $out $in
    description = AR $out

rule link
    command = $cc $linkflags $in -o $out $libs -Iinclude
    description = LINK $out
]]

--

function add_sources(sources)
    local objfiles = {}
    for _, f in pairs(sources) do
        local objfile = f:gsub("%.c", ".o")    
        if f == 'specfuncs' then
            io.write(string.format("build $objdir/%s: cc89 $srcdir/%s\n", objfile, f))
        else
            io.write(string.format("build $objdir/%s: cc $srcdir/%s\n", objfile, f))
        end
        table.insert(objfiles, "$objdir/" .. objfile)
    end
    return objfiles
end

function add_objfiles(objfiles)
    for _, f in pairs(objfiles) do
        io.write("    " .. f .. " $\n")
    end
    io.write("\n")
end


local file = io.open("build.ninja", "w")
io.output(file)
io.write(stub)

-- Build core library
local lib_sources = {'core.c', 'coretests.c', 'entropy.c', 'extratests.c',
    'fileio.c', 'lineardep.c', 'hwtests.c', 'specfuncs.c'}
local lib_objfiles = add_sources(lib_sources)
io.write("build $libdir/libsmokerand_core.a: ar ")
add_objfiles(lib_objfiles)


-- Build batteries
local bat_sources = {'bat_express.c', 'bat_brief.c', 'bat_default.c',
    'bat_file.c', 'bat_full.c'}
local bat_objfiles = add_sources(bat_sources)

-- Build the command line tool executable
io.write("build $objdir/smokerand.o: cc $srcdir/smokerand.c\n")
io.write("build $bindir/smokerand: link $objdir/smokerand.o $libdir/libsmokerand_core.a ")
add_objfiles(bat_objfiles)
io.write("  libs = -L$libdir -lm -lsmokerand_core\n")
io.write("  linkflags=$exe_linkflags\n")

-- Build extra executables
io.write("build $objdir/sr_tiny.o: cc89 $srcdir/sr_tiny.c\n")
io.write("build $bindir/sr_tiny: link $objdir/sr_tiny.o $objdir/specfuncs.o\n")
io.write("  linkflags=$exe_linkflags\n")

io.write("build $objdir/test_funcs.o: cc $srcdir/test_funcs.c\n")
io.write("build $bindir/test_funcs: link $objdir/test_funcs.o $libdir/libsmokerand_core.a\n")
io.write("  libs = -L$libdir -lm -lsmokerand_core\n")
io.write("  linkflags=$exe_linkflags\n")

io.write("build $objdir/calibrate_dc6.o: cc $srcdir/calibrate_dc6.c\n")
io.write("build $bindir/calibrate_dc6: link $objdir/calibrate_dc6.o $libdir/libsmokerand_core.a\n")
io.write("  libs = -L$libdir -lm -lsmokerand_core\n")
io.write("  linkflags=$exe_linkflags\n")


-- Build generators
for _, f in pairs(gen_sources) do
    io.write("build $gen_bindir/obj/lib" .. f .. "_shared.o: cc_gen $gen_srcdir/" .. f .. "_shared.c\n")        
    io.write("build $gen_bindir/lib" .. f .. "_shared.dll: link $gen_bindir/obj/lib" .. f .. "_shared.o\n")
    if f == "crand" then
        io.write("    linkflags = $so_linkflags\n");
    else
        io.write("    linkflags = $gen_linkflags\n");
    end
end

io.close(file)
io.output(io.stdout) 
