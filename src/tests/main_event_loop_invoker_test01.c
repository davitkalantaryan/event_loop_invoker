//
// repo:            event_loop_invoker
// file:			main_event_loop_invoker_test01.c
// path:			src/tests/main_event_loop_invoker_test01.c
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef cinternal_gettid_needed
#define cinternal_gettid_needed
#endif

#include <event_loop_invoker/event_loop_invoker.h>
#include <cinternal/signals.h>
#include <cinternal/gettid.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>


static void* BlockedTestFunction(void* a_pArg);
static void  AsyncTestFunction(void* a_pArg);


int main(int a_argc, char* a_argv[])
{
    int i;
    const int cnMainThreadId = (int)CinternalGetCurrentTid();
    time_t currentTime;
    void *callArg, *pRet;
    struct EvLoopInvokerHandle* invokerHandle;

    CPPUTILS_STATIC_CAST(void, a_argc);
    CPPUTILS_STATIC_CAST(void, a_argv);

    invokerHandle = EvLoopInvokerCreateHandle();
    if (!invokerHandle) {
        fprintf(stderr, "Unable to create event loop invoker\n");
        return 1;
    }

    currentTime = time(&currentTime);
    srand((unsigned int)currentTime);

    fprintf(stdout, "main thread tid: %d\n", cnMainThreadId);

    for (i = 0; i < 30; ++i) {
        CinternalSleepInterruptableMs(10000);
        callArg = (void*)((size_t)rand());
        fprintf(stdout,"main thread (id:%d). Calling blocked finction with arg: %p\n", cnMainThreadId, callArg);
        fflush(stdout);
        pRet = EvLoopInvokerCallFuncionBlocked(invokerHandle, &BlockedTestFunction, callArg);
        fprintf(stdout, "main thread (id:%d). Blocked finction called. pRet: %p\n", cnMainThreadId, pRet);
        CinternalSleepInterruptableMs(2000);
        callArg = (void*)((size_t)rand());
        fprintf(stdout, "main thread (id:%d). Calling async finction with arg: %p\n", cnMainThreadId, callArg);
        fflush(stdout);
        EvLoopInvokerCallFuncionAsync(invokerHandle, &AsyncTestFunction, callArg);
    }

    EvLoopInvokerCleanHandle(invokerHandle);

    return 0;
}


static void* BlockedTestFunction(void* a_pArg)
{
    void* const pRet = (void*)((size_t)rand());
    fprintf(stdout, "Blocked caller threadId: %d, arg: %p, will return: %p\n", (int)CinternalGetCurrentTid(), a_pArg, pRet);
    return pRet;
}


static void  AsyncTestFunction(void* a_pArg)
{
    fprintf(stdout, "Async caller threadId: %d, arg: %p\n", (int)CinternalGetCurrentTid(), a_pArg);
}
