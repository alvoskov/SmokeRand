cmake_minimum_required (VERSION 3.5)
project (SmokeRand)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Compiler settings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROF_FLAGS "")
    set(CMODULE_LINK_FLAGS "-nostdlib")
    set(FREESTANDING "-ffreestanding")
    #set(PROF_FLAGS "-g -pg")
# -ffreestanding 
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
    src/core.c include/smokerand/core.h
    src/coretests.c include/smokerand/coretests.h
    src/entropy.c include/smokerand/entropy.h
    src/extratests.c include/smokerand/extratests.h
    src/lineardep.c include/smokerand/lineardep.h
    src/fileio.c include/smokerand/fileio.h
    include/smokerand/cinterface.h
    include/smokerand/apidefs.h
)
target_include_directories(smokerand_core PRIVATE include)

# Executables
add_executable(smokerand src/smokerand.c
    src/bat_dos16.c include/smokerand/bat_dos16.h
    src/bat_default.c include/smokerand/bat_default.h
    src/bat_brief.c include/smokerand/bat_brief.h
    src/bat_full.c include/smokerand/bat_full.h)
target_include_directories(smokerand PRIVATE include)
target_link_libraries(smokerand smokerand_core)
if (M_LIB)
    target_link_libraries(smokerand m)
endif()

add_executable(test_funcs src/test_funcs.c)
target_include_directories(test_funcs PRIVATE include)
target_link_libraries(test_funcs smokerand_core)
if (M_LIB)
    target_link_libraries(test_funcs m smokerand_core)
endif()


add_executable(calibrate_dc6 src/calibrate_dc6.c)
target_include_directories(calibrate_dc6 PRIVATE include)
target_link_libraries(calibrate_dc6 smokerand_core)
if (M_LIB)
    target_link_libraries(calibrate_dc6 m smokerand_core)
endif()



# Examples of plugins, i.e. modules with PRNGs
foreach(prng_name alfib alfib_mod chacha chacha_avx coveyou64 drand48
    isaac64 kiss64 kiss93 kiss99
    lcg32prime
    lcg69069 lcg64 lcg64prime lcg96 lcg128 lcg128_full lcg128_u32_full
    loop_7fff_w64
    lfib_par lfsr113 lfsr258 minstd mt19937 mlfib17_5 mrg32k3a
    msws mulberry32
    mwc64 mwc64x mwc128 mwc128x
    r1279 rc4 rrmxmx pcg32 pcg64 philox philox32 randu romutrio sezgin63
    sfc32 sfc64 shr3 superduper73 superduper64 superduper64_u32
    swb swblux swbw
    speck128 speck128_avx splitmix splitmix32 sqxor sqxor32
    tinymt32 tinymt64 threefry well1024a wyrand
    xorshift128 xorshift128p xorwow
    xoroshiro128p xoroshiro128pp xoroshiro1024st xoroshiro1024stst xsh
    crand)
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

# RANLUX++
add_library(ranluxpp_shared SHARED generators/ranluxpp_shared.c generators/ranluxpp_helpers.h
    generators/ranluxpp_mulmod.h include/smokerand/cinterface.h)
set_target_properties(ranluxpp_shared PROPERTIES LINK_FLAGS ${CMODULE_LINK_FLAGS})
set_target_properties(ranluxpp_shared PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
set_target_properties(ranluxpp_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
target_include_directories(ranluxpp_shared PRIVATE include)
