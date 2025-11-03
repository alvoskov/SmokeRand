/**
 * @file threads_intf.c
 * @brief Cross-platform multithreading interface. Supports POSIX threads
 * and WinAPI threads.
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/threads_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NTHREADS_MAX 128

static ThreadObj threads[NTHREADS_MAX];
static int nthreads = 0;
DECLARE_MUTEX(thread_ord_mutex)


void init_thread_dispatcher(void)
{
    INIT_MUTEX(thread_ord_mutex);
    nthreads = 0;
}

/**
 * @brief Creates and runs a new thread.
 * @param thr_func  Thread function
 * @param udata     Pointer to a user defined data sent to a thread function.
 * @param ord       Thread ordinal used in diagnostic messages.
 */
ThreadObj ThreadObj_create(ThreadFuncPtr thr_func, void *udata, unsigned int ord)
{
    ThreadObj obj;
    obj.ord = ord;
    obj.exists = 1;
#ifdef USE_PTHREADS
    pthread_create(&obj.id, NULL, thr_func, udata);
    // Get data from threads
#elif defined(USE_WINTHREADS)
    obj.handle = CreateThread(NULL, 0, thr_func, udata, 0, &obj.id);
#else
    obj.id = ord;
    thr_func(udata);
#endif

    MUTEX_LOCK(thread_ord_mutex);
    if (nthreads < NTHREADS_MAX) {
        threads[nthreads++] = obj;
    }
    MUTEX_UNLOCK(thread_ord_mutex);
    return obj;
}

/**
 * @brief Wait for thread completion.
 */
void ThreadObj_wait(ThreadObj *obj)
{
#ifdef USE_PTHREADS
    pthread_join(obj->id, NULL);
#elif defined(USE_WINTHREADS)
    WaitForSingleObject(obj->handle, INFINITE);
#else
    (void) obj;
#endif
    for (int i = 0; i < nthreads; i++) {
        if (obj->id == threads[i].id && threads[i].exists) {
            threads[i].exists = 0;
        }
    }
}

/**
 * @brief Get information about the current thread.
 */
ThreadObj ThreadObj_current(void)
{
    ThreadObj obj;
#ifdef USE_PTHREADS
    obj.id = pthread_self();
#elif defined(USE_WINTHREADS)
    obj.id = GetCurrentThreadId();
#else
    obj.id = 1;
#endif
    for (int i = 0; i < nthreads; i++) {
        if (threads[i].id == obj.id && threads[i].exists) {
            return threads[i];
        }
    }
    obj.ord = 1;
    return obj;
}

//-------------------------------------------------------------

// Uncomment if you want to use PE32 loader instead of DXE3 loader in DJGPP
// #ifdef __DJGPP__
// #define USE_PE32_DOS
// #endif

#ifdef USE_LOADLIBRARY
    #include <windows.h>
#elif defined(USE_PE32_DOS)
    #include "smokerand/pe32loader.h"
#elif !defined(NO_POSIX)
    #include <unistd.h>
    #include <dlfcn.h>
#endif

void *dlopen_wrap(const char *libname)
{
#ifdef USE_LOADLIBRARY
    HMODULE lib = LoadLibraryA(libname);
    if (lib == NULL || lib == INVALID_HANDLE_VALUE) {
        int errcode = (int) GetLastError();
        fprintf(stderr, "Cannot load the '%s' module; error code: %d\n",
            libname, errcode);
        LPSTR msg_buf = NULL;
        (void) FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errcode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &msg_buf, 0, NULL);
        fprintf(stderr, "  Error message: %s\n", msg_buf);
        LocalFree(msg_buf);
        lib = 0;
    }
    return (void *) lib;
#elif defined(USE_PE32_DOS)
    void *lib = dlopen_pe32dos(libname, 0);
    if (lib == NULL) {
        fprintf(stderr, "dlopen() error: %s\n", dlerror_pe32dos());
    };
    return lib;
#elif !defined(NO_POSIX)
    void *lib = dlopen(libname, RTLD_LAZY);
    if (lib == NULL) {
        fprintf(stderr, "dlopen() error: %s\n", dlerror());
    };
    return lib;
#else
    fprintf(stderr,
        "Cannot load the '%s' module: shared libraries support not found!\n",
        libname);
    return NULL;
#endif

}


void *dlsym_wrap(void *handle, const char *symname)
{
#ifdef USE_LOADLIBRARY
    return (void *) GetProcAddress(handle, symname);
#elif defined(USE_PE32_DOS)
    return dlsym_pe32dos(handle, symname);
#elif defined(__DJGPP__)
    size_t len = strlen(symname);
    char *mangled_symname = calloc(len + 2, sizeof(char));    
    mangled_symname[0] = '_';
    memcpy(mangled_symname + 1, symname, len);
    void *lib = dlsym(handle, mangled_symname);
    free(mangled_symname);
    if (lib == NULL) {
        fprintf(stderr, "dlopen() error: %s\n", dlerror());
    };
    return lib;
#elif !defined(NO_POSIX)
    return dlsym(handle, symname);
#else
    return NULL;
#endif
}


void dlclose_wrap(void *handle)
{
#ifdef USE_LOADLIBRARY
    FreeLibrary((HANDLE) handle);
#elif defined(USE_PE32_DOS)
    dlclose_pe32dos(handle);
#elif !defined(NO_POSIX)
    dlclose(handle);
#else
    (void) handle;
#endif
}


int get_cpu_numcores(void)
{
#ifdef USE_LOADLIBRARY
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif defined(__DJGPP__)
    return 1;
#elif !defined(NO_POSIX)
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 1;
#endif
}

//-------------------------------------------------------------


long long get_ram_size(void)
{
#ifdef USE_LOADLIBRARY
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return (long long) statex.ullTotalPhys;
#elif !defined(NO_POSIX)
    const long long pages = sysconf(_SC_PHYS_PAGES);
    const long long page_size = sysconf(_SC_PAGESIZE);
    const long long total_ram_bytes = pages * page_size;
    return total_ram_bytes;
#else
    return -1;
#endif
}
