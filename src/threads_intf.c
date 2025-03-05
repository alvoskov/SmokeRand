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

