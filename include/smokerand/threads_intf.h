/**
 * @file threads_intf.h
 * @brief Cross-platform multithreading interface. Supports POSIX threads
 * and WinAPI threads.
 * @details Rules of selection of default multi-threading API:
 * 
 * 1. MSVC - use WinAPI threads (POSIX threads are usually not present here)
 * 2. CLANG or GCC/MinGW - use POSIX threads.
 *
 * Can be overriden by `-dNOTHREADS` (stubs instead of multithreading API)
 * or by `-dUSE_WINTHREADS` (use WinAPI threads).
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_THREADS_INTF_H
#define __SMOKERAND_THREADS_INTF_H
#include "smokerand/coredefs.h"

////////////////////////////////////////////////////
///// Selection of default multi-threading API /////
////////////////////////////////////////////////////

#ifndef NOTHREADS
    #if defined(_MSC_VER) && !defined (__clang__)
    #define USE_WINTHREADS
    #endif

    #if (defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)) && !defined(__clang__) && !defined(USE_WINTHREADS)
    #define USE_PTHREADS
    #endif
#endif

//////////////////////////////////
///// Platform-specific part /////
//////////////////////////////////

#if defined(USE_PTHREADS)
//---------------------------------------------------------------------------------
// pthreads version
#include <pthread.h>
#define DECLARE_MUTEX(mutex) static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define INIT_MUTEX(mutex) pthread_mutex_init(&(mutex), NULL);
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&(mutex));
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&(mutex));
#define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&(mutex));

typedef struct {
    pthread_t id;
    unsigned int ord;
    int exists;
} ThreadObj;

typedef void* ThreadRetVal;
#define THREADFUNC_SPEC

//---------------------------------------------------------------------------------
// WinAPI version
#elif defined(USE_WINTHREADS)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define DECLARE_MUTEX(mutex) static HANDLE mutex = NULL;
#define INIT_MUTEX(mutex) if (mutex == NULL) { mutex = CreateMutex(NULL, FALSE, "\""#mutex"\""); }
#define MUTEX_LOCK(mutex) { \
    DWORD dwResult = WaitForSingleObject(mutex, INFINITE); \
    if (dwResult != WAIT_OBJECT_0) { \
        fprintf(stderr, "get_seed64_mt internal error"); \
        exit(EXIT_FAILURE); \
    } \
}
#define MUTEX_UNLOCK(mutex) ReleaseMutex(mutex);
#define MUTEX_DESTROY(mutex) CloseHandle(mutex);

typedef struct {
    DWORD id;
    HANDLE handle;
    unsigned int ord;
    int exists;
} ThreadObj;

typedef DWORD ThreadRetVal;
#define THREADFUNC_SPEC WINAPI

//---------------------------------------------------------------------------------
// Stubs for a plaform without threads
#else
//#pragma message ("Note: no known multithreading API is present")
#include <stdint.h>

#define DECLARE_MUTEX(mutex)
#define INIT_MUTEX(mutex)
#define MUTEX_LOCK(mutex)
#define MUTEX_UNLOCK(mutex)
#define MUTEX_DESTROY(mutex)

typedef struct {
    uint64_t id;
    unsigned int ord;
    int exists;
} ThreadObj;

typedef int ThreadRetVal;
#define THREADFUNC_SPEC
#endif

///////////////////////////////
///// Cross-platform part /////
///////////////////////////////

typedef ThreadRetVal (THREADFUNC_SPEC *ThreadFuncPtr)(void *);

void init_thread_dispatcher(void);
ThreadObj ThreadObj_create(ThreadFuncPtr thr_func, void *udata, unsigned int ord);
void ThreadObj_wait(ThreadObj *obj);
ThreadObj ThreadObj_current(void);

//------------------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#define USE_LOADLIBRARY
#elif defined(__WATCOMC__) && defined(__386__) && defined(__DOS__)
#define USE_PE32_DOS
#endif

void *dlopen_wrap(const char *libname);
void *dlsym_wrap(void *handle, const char *symname);
void dlclose_wrap(void *handle);
unsigned int get_cpu_numcores(void);
int get_ram_info(RamInfo *info);

#endif // __SMOKERAND_THREADS_INTF_H
