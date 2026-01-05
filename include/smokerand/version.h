/**
 * @file version.h
 * @brief Some macro definitions regarding to SmokeRand version, used compiled
 * and platform.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_VERSION_H
#define __SMOKERAND_VERSION_H
#include <limits.h>

/**
 * @brief SmokeRand version number (set manually)
 */
#define SMOKERAND_VERSION "0.45"

//////////////////////////////////
///// Compiler autodetection /////
//////////////////////////////////

#if defined(__clang__)
    #define SMOKERAND_COMPILER "clang"
#elif defined(__DJGPP__)
    #define SMOKERAND_COMPILER "djgpp"
#elif defined(__MSYS__)
    #define SMOKERAND_COMPILER "msys"
#elif defined(__WATCOMC__)
    #define SMOKERAND_COMPILER "watcom"
#elif defined(_MSC_VER)
    #define SMOKERAND_COMPILER "msvc"
#elif defined(__MINGW32__) || defined(__MINGW64__)
    #define SMOKERAND_COMPILER "mingw"
#elif defined(__GNUC__)
    #define SMOKERAND_COMPILER "gcc"
#else
    #define SMOKERAND_COMPILER "unknown"
#endif

///////////////////////////////////////
///// Platfrom (OS) autodetection /////
///////////////////////////////////////

#ifdef _WIN64
    #define SMOKERAND_OS "win64"
#elif defined(_WIN32)
    #define SMOKERAND_OS "win32"
#elif defined(__DOS__) || defined(__DJGPP__)
    #define SMOKERAND_OS "dos"
#elif defined(__linux__)
    #if SIZE_MAX == UINT32_MAX
        #define SMOKERAND_OS "linux32"
    #else
        #define SMOKERAND_OS "linux64"
    #endif
#elif defined(__unix__)
    #if SIZE_MAX == UINT32_MAX
        #define SMOKERAND_OS "unix32"
    #else
        #define SMOKERAND_OS "unix64"
    #endif
#else
    #if SIZE_MAX == UINT32_MAX
        #define SMOKERAND_OS "generic32"
    #else
        #define SMOKERAND_OS "generic64"
    #endif
#endif

/**
 * @brief SmokeRand version with compiler and platform names.
 */
#define SMOKERAND_VERSION_FULL (SMOKERAND_VERSION "-" SMOKERAND_COMPILER "-" SMOKERAND_OS)


#endif // __SMOKERAND_VERSION_H
