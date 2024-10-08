cmake_minimum_required (VERSION 3.5)
project (SmokeRand)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Compiler settings
if (NOT MSVC)
    set(PROF_FLAGS "")
    set(CMODULE_LINK_FLAGS "-nostdlib")
    #set(PROF_FLAGS "-g -pg")
# -ffreestanding 
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROF_FLAGS} -std=c99 -O2 -Wall -Werror -Wextra -Wno-attributes -march=native")
else()
    set(CMODULE_LINK_FLAGS " ")
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

# Executables
add_executable(smokerand src/smokerand.c
    src/smokerand_core.c include/smokerand/smokerand_core.h
    include/smokerand/cinterface.h)
target_include_directories(smokerand PRIVATE include)

# Examples of plugins, i.e. modules with PRNGs
foreach(prng_name alfib lcg69069 lcg64 lcg128 mwc64 rrmxmx sezgin63)
    add_library(${prng_name}_shared SHARED generators/${prng_name}_shared.c include/smokerand/cinterface.h)
    set_target_properties(${prng_name}_shared PROPERTIES LINK_FLAGS ${CMODULE_LINK_FLAGS})
    set_target_properties(${prng_name}_shared PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    set_target_properties(${prng_name}_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/generators)
    target_include_directories(${prng_name}_shared PRIVATE include)
endforeach()
