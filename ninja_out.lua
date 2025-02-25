--
-- Creates build.ninja file for SmokeRand.
--

local platform = ''
if #arg < 1 then
    print(
    [[Usage:
    lua ninja_out.lua plaform
    platform --- gcc, mingw, msvc
    ]])
    return 0
else
    platform = arg[1]
end

local lib_name = "$libdir/libsmokerand_core.a"

local lib_sources = {'core.c', 'coretests.c', 'entropy.c', 'extratests.c',
    'fileio.c', 'lineardep.c', 'hwtests.c', 'specfuncs.c'}

local bat_sources = {'bat_express.c', 'bat_brief.c', 'bat_default.c',
    'bat_file.c', 'bat_full.c'}

local gen_sources = {'aesni', 'alfib_lux', 'alfib_mod', 'alfib', 'ara32',
    'chacha_avx',  'chacha', 'cmwc4096', 'coveyou64', 'crand', 'cwg64', 'des',
    'drand48', 'efiix64x48', 'flea32x1', 'hc256', 'isaac64', 'kiss64', 'kiss93',
    'kiss99', 'lcg128_full', 'lcg128', 'lcg128_u32_full',
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

local exe_ext, stub, stub_crossplatform = '', '', [[
srcdir = src
objdir = obj
bindir = bin
libdir = lib
includedir = include/smokerand
gen_bindir = $bindir/generators
gen_srcdir = generators
]]

if platform == 'gcc' or platform == 'mingw' then
    if platform == 'mingw' then
        exe_ext = '.exe'
    end
    stub = [[cflags = -std=c99 -O3 -Werror -Wall -Wextra -Wno-attributes -march=native
cflags89 = -std=c89 -O3 -Werror -Wall -Wextra -Wno-attributes -march=native
gen_cflags = $cflags -fPIC -ffreestanding -nostdlib
cc = gcc
exe_libs =
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
elseif platform == 'msvc' then
    exe_ext = '.exe'
    -- /WX if treat warnings as errors
    stub = [[cflags = /O2 /W3 /arch:avx2 /D_CRT_SECURE_NO_WARNINGS
cflags89 = /O2 /W3 /arch:avx2 /D_CRT_SECURE_NO_WARNINGS
gen_cflags = $cflags
cc = cl
exe_libs = advapi32.lib
exe_linkflags = /SUBSYSTEM:CONSOLE
so_linkflags = /DLL
gen_linkflags = /DLL
]]

--    stub = stub .. "\nmsvc_deps_prefix = Note: including file:\n"
    stub = stub .. "\nmsvc_deps_prefix = Примечание: включение файла:\n"

    stub = stub .. [[
rule cc
    command = $cc /showIncludes $cflags /c /Iinclude $in /Fo:$out
    description = CC $out
    deps = msvc

rule cc89
    command = $cc /showIncludes $cflags /c /Iinclude $in /Fo:$out
    description = CC $out
    deps = msvc

rule cc_gen
    command = $cc /showIncludes $cflags /c /Iinclude $in /Fo:$out
    description = CC $out
    deps = msvc

rule ar
    command = lib /OUT:$out $in
    description = AR $out

rule link
    command = link $linkflags $in $libs /Out:$out
    description = LINK $out
]]
else
    error("Unknown platform '" .. platform .. "'")
    return 1
end

print("Generating build.ninja for the '" .. platform .. "'...")

stub = stub_crossplatform .. "\n" .. stub .. "\n"

local default_builds = {}
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
    io.write("\n");
end

function add_exefile(exefile, extra_objfiles)
    io.write("build $objdir/" .. exefile .. ".o: cc $srcdir/" .. exefile .. ".c\n")
    local exefile_full = "$bindir/" .. exefile .. exe_ext
    io.write("build " .. exefile_full .. ": link $objdir/" .. exefile .. ".o ")
    table.insert(default_builds, exefile_full)
    if #extra_objfiles > 0 then
        add_objfiles(extra_objfiles)
    else
        io.write("\n");
    end
    io.write("  libs = $exe_libs\n")
    io.write("  linkflags = $exe_linkflags\n")
end

local file = io.open("build.ninja", "w")
io.output(file)
io.write(stub)

-- Build core library
local lib_objfiles = add_sources(lib_sources)
io.write("build " .. lib_name .. ": ar ")
add_objfiles(lib_objfiles)

-- Build batteries
local bat_objfiles = add_sources(bat_sources)

-- Build the command line tool executable
table.insert(bat_objfiles, lib_name)
add_exefile("smokerand", bat_objfiles)
add_exefile("test_funcs", {lib_name})
add_exefile("calibrate_dc6", {lib_name})
-- Build extra executables
io.write("build $objdir/sr_tiny.o: cc89 $srcdir/sr_tiny.c\n")
io.write("build $bindir/sr_tiny" .. exe_ext .. ": link $objdir/sr_tiny.o $objdir/specfuncs.o\n")
io.write("  linkflags=$exe_linkflags\n")
table.insert(default_builds, "$bindir/sr_tiny" .. exe_ext)

-- Build generators
for _, f in pairs(gen_sources) do
    local gen_fullname = "$gen_bindir/lib" .. f .. "_shared.dll"
    io.write("build $gen_bindir/obj/" .. f .. "_shared.o: cc_gen $gen_srcdir/" .. f .. "_shared.c\n")        
    io.write("build " .. gen_fullname .. ": link $gen_bindir/obj/" .. f .. "_shared.o\n")
    table.insert(default_builds, gen_fullname)
    if f == "crand" then
        io.write("    linkflags = $so_linkflags\n\n");
    else
        io.write("    linkflags = $gen_linkflags\n\n");
    end
end

-- Default rules
io.write("default ")
for k, v in pairs(default_builds) do        
    io.write(v .. " ")
    if k % 2 == 0 then
        io.write("$\n    ")
    end
end
io.write("\n\n")


io.close(file)
io.output(io.stdout) 

print("Success")

