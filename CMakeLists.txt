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

# Check if math library is required to link
# It is a workaround for different behaviour of GCC and MSVC
# GCC in GNU/Linux cannot work without -lm key, MSVC
# cannot work WITH it, MinGW tolerates both variants
find_library(M_LIB m)

add_library(smokerand_core
    src/core.c       include/smokerand/core.h
    src/coretests.c  include/smokerand/coretests.h
    src/entropy.c    include/smokerand/entropy.h
    src/extratests.c include/smokerand/extratests.h
    src/lineardep.c  include/smokerand/lineardep.h
    src/fileio.c     include/smokerand/fileio.h
    src/hwtests.c    include/smokerand/hwtests.h
    src/specfuncs.c  include/smokerand/specfuncs.h
    include/smokerand/cinterface.h
    include/smokerand/apidefs.h
)
target_include_directories(smokerand_core PRIVATE include)

# Executables
# a) Main executable
add_executable(smokerand src/smokerand.c
    src/bat_express.c include/smokerand/bat_express.h
    src/bat_default.c include/smokerand/bat_default.h
    src/bat_brief.c   include/smokerand/bat_brief.h
    src/bat_file.c    include/smokerand/bat_file.h
    src/bat_full.c    include/smokerand/bat_full.h)
target_include_directories(smokerand PRIVATE include)
target_link_libraries(smokerand smokerand_core)
if (M_LIB)
    target_link_libraries(smokerand m)
endif()

# b) Some tests
set(TESTS calibrate_dc6 test_funcs test_rdseed)
foreach(testname ${TESTS})
    add_executable(${testname} src/${testname}.c)
    target_include_directories(${testname} PRIVATE include)
    target_link_libraries(${testname} smokerand_core)
    if (M_LIB)
        target_link_libraries(${testname} m smokerand_core)
    endif()
endforeach()

# Generators list
if (ONLY_PORTABLE)
    message("Only portable generators will be compiled")
    set(GENERATORS alfib_lux alfib_mod alfib ara32 chacha cmwc4096 coveyou64 crand 
    cwg64 des drand48 efiix64x48 flea32x1 hc256 isaac64 kiss64 kiss93 
    kiss99 lcg128_u32_portable lcg32prime lcg64 lcg69069 lcg96_portable 
    lfib4 lfib4_u64 lfib_par lfsr113 lfsr258 loop_7fff_w64 lrnd64 
    lxm_64x128 macmarsa magma minstd mixmax mlfib17_5 mrg32k3a msws_ctr 
    msws mt19937 mulberry32 mwc1616x mwc1616 mwc3232x mwc32x mwc4691 
    mwc64x mwc64 pcg32 pcg32_streams pcg32_xsl_rr pcg64_64 philox2x32 
    philox32 r1279 randu ranlux48 ranluxpp ranq1 ranq2 ranrot32 ranrot_bi 
    ranshi ranval ran rc4ok rc4 romutrio rrmxmx sapparot2 sapparot 
    sezgin63 sfc16 sfc32 sfc64 sfc8 shr3 speck128 splitmix32 splitmix 
    sqxor32 stormdrop_old stormdrop superduper64 superduper64_u32 
    superduper73 swblarge swblux swbw swb taus88 threefry2x64 threefry 
    tinymt32 tinymt64 well1024a xorgens xoroshiro1024stst xoroshiro1024st 
    xoroshiro128pp xoroshiro128p xorshift128p xorshift128 xorwow 
    xsh)
else()
    message("All generators will be compiled")
    set(GENERATORS aesni alfib_lux alfib_mod alfib ara32 chacha_avx chacha cmwc4096 
    coveyou64 crand cwg64 des drand48 efiix64x48 flea32x1 hc256 isaac64 
    kiss64 kiss93 kiss99 lcg128_full lcg128 lcg128_u32_full lcg128_u32_portable 
    lcg32prime lcg64prime lcg64 lcg69069 lcg96_portable lcg96 lfib4 
    lfib4_u64 lfib_par lfsr113 lfsr258 loop_7fff_w64 lrnd64 lxm_64x128 
    macmarsa magma_avx magma minstd mixmax mlfib17_5 mrg32k3a msws_ctr 
    msws mt19937 mulberry32 mwc128x mwc128 mwc1616x mwc1616 mwc3232x 
    mwc32x mwc4691 mwc64x mwc64 pcg32 pcg32_streams pcg32_xsl_rr 
    pcg64_64 pcg64_xsl_rr philox2x32 philox32 philox r1279 randu 
    ranlux48 ranluxpp ranq1 ranq2 ranrot32 ranrot_bi ranshi ranval 
    ran rc4ok rc4 romutrio rrmxmx sapparot2 sapparot sezgin63 sfc16 
    sfc32 sfc64 sfc8 shr3 speck128_avx speck128_r16_avx speck128 
    splitmix32 splitmix sqxor32 sqxor stormdrop_old stormdrop superduper64 
    superduper64_u32 superduper73 swblarge swblux swbw swb taus88 
    threefry2x64_avx threefry2x64 threefry tinymt32 tinymt64 well1024a 
    wyrand xorgens xoroshiro1024stst xoroshiro1024st xoroshiro128pp_avx 
    xoroshiro128pp xoroshiro128p xorshift128pp_avx xorshift128p xorshift128 
    xorwow xsh)
endif()


# Examples of plugins, i.e. modules with PRNGs
foreach(prng_name ${GENERATORS})
    add_library(${prng_name}_shared SHARED generators/${prng_name}_shared.c include/smokerand/cinterface.h)
    if (NOT ${prng_name} STREQUAL "crand")
        # crand uses standard library, so we can't link it as a freestanding
        # appication. All other modules don't use standard library and are
        # freestanding.
        set_target_properties(${prng_name}_shared PROPERTIES LINK_FLAGS ${CMODULE_LINK_FLAGS})
        target_compile_options(${prng_name}_shared PRIVATE ${FREESTANDING}) 
    endif()
    set_target_properties(${prng_name}_shared PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    set_target_properties(${prng_name}_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    set_target_properties(${prng_name}_shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    target_include_directories(${prng_name}_shared PRIVATE include)
endforeach()
