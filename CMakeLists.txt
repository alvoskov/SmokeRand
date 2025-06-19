cmake_minimum_required (VERSION 3.5)
project (SmokeRand)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Compiler settings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROF_FLAGS "")
    set(CMODULE_LINK_FLAGS "-nostdlib")
    set(FREESTANDING "-ffreestanding")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROF_FLAGS} -std=c99 -O2 -Wall -Werror -Wextra -Wno-attributes -march=native")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "OpenWatcom")
    set(CMODULE_LINK_FLAGS " ")
    set(FREESTANDING " ")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -za99 -DNO_X86_EXTENSIONS -DNOTHREADS")
else()
    set(CMODULE_LINK_FLAGS " ")
    set(FREESTANDING " ")
endif()

# Some CMake settings
set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SHARED_LIBRARY_PREFIX )
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Check if math library is required to link
# It is a workaround for different behaviour of GCC and MSVC
# GCC in GNU/Linux cannot work without -lm key, MSVC
# cannot work WITH it, MinGW tolerates both variants
find_library(M_LIB m)
if (M_LIB)
    set(COMMON_LIBS m)
endif()

add_library(smokerand_core
    src/core.c          include/smokerand/core.h
    src/coretests.c     include/smokerand/coretests.h
    src/entropy.c       include/smokerand/entropy.h
    src/extratests.c    include/smokerand/extratests.h
    src/lineardep.c     include/smokerand/lineardep.h
    src/fileio.c        include/smokerand/fileio.h
    src/hwtests.c       include/smokerand/hwtests.h
    src/specfuncs.c     include/smokerand/specfuncs.h
    src/threads_intf.c  include/smokerand/threads_intf.h
    include/smokerand/apidefs.h
    include/smokerand/cinterface.h
    include/smokerand/int128defs.h
    include/smokerand/x86exts.h
    include/smokerand_core.h
)
target_include_directories(smokerand_core PRIVATE include)

add_library(smokerand_bat
    src/bat_express.c include/smokerand/bat_express.h
    src/bat_default.c include/smokerand/bat_default.h
    src/bat_brief.c   include/smokerand/bat_brief.h
    src/bat_file.c    include/smokerand/bat_file.h
    src/bat_full.c    include/smokerand/bat_full.h
    src/bat_special.c include/smokerand/bat_special.h
    include/smokerand_bat.h)
target_include_directories(smokerand_bat PRIVATE include)

# Executables
# a) Main executable
add_executable(smokerand apps/smokerand.c)
target_include_directories(smokerand PRIVATE include)
target_link_libraries(smokerand smokerand_bat smokerand_core ${COMMON_LIBS})

# b) Simplified version
add_executable(sr_tiny apps/sr_tiny.c src/specfuncs.c)
target_include_directories(sr_tiny PRIVATE include)
target_link_libraries(sr_tiny ${COMMON_LIBS})

# c) PE loader for 32-bit systems
add_executable(peparse apps/peparse.c src/pe32loader.c)
target_include_directories(peparse PRIVATE include)
target_link_libraries(peparse smokerand_core ${COMMON_LIBS})

# d) Some tests
set(TESTS calibrate_dc6 calibrate_linearcomp test_funcs test_rdseed)
foreach(testname ${TESTS})
    add_executable(${testname} apps/${testname}.c)
    target_include_directories(${testname} PRIVATE include)
    target_link_libraries(${testname} smokerand_core ${COMMON_LIBS})
endforeach()

# e) Call batteries from C++
add_executable(test_cpp11 apps/test_cpp11.cpp)
target_include_directories(test_cpp11 PRIVATE include)
target_link_libraries(test_cpp11 smokerand_bat smokerand_core ${COMMON_LIBS})

# f) crand uses standard library, so we can't link it as a freestanding
# dynamic linked (shared) library.
add_executable(test_crand apps/test_crand.c)
target_include_directories(test_crand PRIVATE include)
target_link_libraries(test_crand smokerand_bat smokerand_core ${COMMON_LIBS})

# Generators list
set (GENERATORS
    aes128 alfib alfib64x5 alfib8x5 alfib_lux alfib_mod ara32 biski16 
    biski16_alt biski16_mul biski32 biski32_alt biski64 biski64sh 
    biski64_alt biski64_mul biski8 biski8_alt biski8_mul chacha cmwc4096 
    cmwc4827 coveyou64 cswb4288 cswb4288_64 cwg64 des drand48 efiix64x48 
    flea32x1 gjrand64 gmwc128 hc256 hicg64 icg31x2 ia ia64 ibaa icg64 
    icg64_p2 isaac64 jkiss jkiss32 jlkiss64 kiss11_32 kiss11_64 kiss64 
    kiss93 kiss99 kiss4691 kuzn lcg128 lcg32prime lcg64 lcg64prime 
    lcg69069 lcg96 lea lfib4 lfib4_u64 lfib_par lfib_ranmar lfsr113 
    lfsr258 loop_7fff_w64 lrnd64_1023 lrnd64_255 lxm_64x128 macmarsa 
    magma melg19937 melg44497 melg607 minstd mixmax mlfib17_5 mrg32k3a 
    msws msws64 msws64x msws_ctr mt19937 mularx128 mularx128_str 
    mularx128_u32 mularx256 mularx64_r2 mularx64_u32 mulberry32 mwc128 
    mwc128x mwc128xxa32 mwc1616 mwc1616x mwc16_32 mwc256xxa64 mwc3232x 
    mwc32x mwc32xxa8 mwc40xxa8 mwc4691 mwc48xxa16 mwc64 mwc64x mwc64_2p58 
    mwc8222 pcg32 pcg32_xsl_rr pcg64_64 pcg64_xsl_rr philox philox2x32 
    philox32 r1279 r250 ran randu ranlux48 ranluxpp ranq1 ranq2 ranrot32 
    ranrot_bi ranshi ranval ranval64 rc4 rc4ok romutrio rrmxmx sapparot 
    sapparot2 sapparot2_64 sezgin63 sfc16 sfc32 sfc64 sfc8 shr3 skiss32 
    skiss64 smwc16x8 smwc192 smwc192lux smwc24 smwc48 smwc48lux smwc8x16 
    smwc96 speck128 speck128sc speck64_128 splitmix splitmix32 splitmix32cbc 
    splitmix_g1 sqxor sqxor32 stormdrop superduper64 superduper73 
    superduper96 swb swblarge swblux swbw taus88 threefry threefry2x64 
    tinymt32 tinymt64 tylo64 v3b well1024a weyl wich1982 wich2006 
    wob2m wyrand xabc16 xabc32 xabc64 xabc8 xkiss8_awc xkiss16_awc 
    xkiss32_awc xkiss32sh_awc xorgens xoroshiro1024st xoroshiro1024stst 
    xoroshiro128 xoroshiro128p xoroshiro128pp xoroshiro128pp_vec 
    xoroshiro32pp xoroshiro64pp xoroshiro64st xoroshiro64stst xorshift128 
    xorshift128p xorshift128pp xorshift64 xorshift64st xorwow xoshiro128p 
    xoshiro128pp xsh xtea xxtea ziff98 
)

# Examples of plugins, i.e. modules with PRNGs
foreach(prng_name ${GENERATORS})
    add_library(${prng_name} SHARED generators/${prng_name}.c include/smokerand/cinterface.h)
    set_target_properties(${prng_name} PROPERTIES LINK_FLAGS ${CMODULE_LINK_FLAGS})
    target_compile_options(${prng_name} PRIVATE ${FREESTANDING}) 
    set_target_properties(${prng_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    set_target_properties(${prng_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    set_target_properties(${prng_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    target_include_directories(${prng_name} PRIVATE include)
endforeach()
