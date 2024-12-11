//
// repo:            event_loop_invoker
// file:			main_event_loop_invoker_platform_test01.c
// path:			src/tests/main_event_loop_invoker_platform_test01.c
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef cinternal_gettid_needed
#define cinternal_gettid_needed
#endif

#include <event_loop_invoker/event_loop_invoker_platform.h>
#include <cinternal/signals.h>
#include <cinternal/gettid.h>
#include <cinternal/signals.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <cinternal/undisable_compiler_warnings.h>


static struct EvLoopInvokerEventsMonitor* RegisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, void* a_pData);
static void UnregisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, struct EvLoopInvokerEventsMonitor* a_monitor);
static int s_nMainThreadId;


int main(int a_argc, char* a_argv[])
{
    struct EvLoopInvokerHandle* invokerHandle;
    struct EvLoopInvokerEventsMonitor* pMonitor;
    
    CPPUTILS_STATIC_CAST(void, a_argc);
    CPPUTILS_STATIC_CAST(void, a_argv);

    s_nMainThreadId = (int)CinternalGetCurrentTid();
    fprintf(stdout, "mainThreadId = %d\n", s_nMainThreadId);

    invokerHandle = EvLoopInvokerCreateHandle();
    if (!invokerHandle) {
        fprintf(stderr, "Unable to create event loop invoker\n");
        return 1;
    }

    pMonitor = RegisterMonitor(invokerHandle,CPPUTILS_NULL);
    if (!invokerHandle) {
        EvLoopInvokerCleanHandle(invokerHandle);
        fprintf(stderr, "Unable to register monitor\n");
        return 1;
    }

    CinternalSleepInterruptableMs(5000);

    UnregisterMonitor(invokerHandle, pMonitor);
    EvLoopInvokerCleanHandle(invokerHandle);

    return 0;
}


static int s_nCreatorThreadId;


#ifdef _WIN32

static bool EvLoopInvokerTestEventMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, void* a_pData, MSG* a_msg)
{
    const int nCallerThreadId = (int)CinternalGetCurrentTid();
    (void)a_invokerHandle;
    fprintf(stdout, "mainThreadId = %d, creatorThreadId = %d, callerThreadId = %d => data:%p, msg(wparam:%d, lparam:%d)\n", 
        s_nMainThreadId, s_nCreatorThreadId, nCallerThreadId, a_pData, (int)a_msg->wParam, (int)a_msg->lParam);
    return false;
}

#elif defined(__linux__) || defined(__linux)

static bool EvLoopInvokerTestEventMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle,void* a_pData, xcb_generic_event_t* a_msg)
{
    const int nCallerThreadId = (int)CinternalGetCurrentTid();
    (void)a_invokerHandle;
    fprintf(stdout, "mainThreadId = %d, creatorThreadId = %d, callerThreadId = %d => data:%p, msg(response_type:%d)\n",
        s_nMainThreadId, s_nCreatorThreadId, nCallerThreadId, a_pData, (int)a_msg->response_type);
    return false;
}


#elif defined(__APPLE__)

static bool EvLoopInvokerTestEventMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle,void* a_pData, MSG* a_msg)
{
    const int nCallerThreadId = (int)CinternalGetCurrentTid();
    (void)a_invokerHandle;
    fprintf(stdout, "mainThreadId = %d, creatorThreadId = %d, callerThreadId = %d => data:%p, msg(wparam:%d, lparam:%d)\n",
        s_nMainThreadId, s_nCreatorThreadId, nCallerThreadId, a_pData, (int)a_msg->wParam, (int)a_msg->lParam);
    return false;
}


#else
# platform is not supported
#endif



static void* BlockedFunctionToRegisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, void* a_pArg)
{
    struct EvLoopInvokerEventsMonitor* const pMonitor = EvLoopInvokerRegisterEventsMonitor(a_invokerHandle,&EvLoopInvokerTestEventMonitor, a_pArg);
    s_nCreatorThreadId = (int)CinternalGetCurrentTid();
    fprintf(stdout, "mainThreadId = %d, creatorThreadId = %d\n", s_nMainThreadId, s_nCreatorThreadId);
    return pMonitor;
}


static void* BlockedFunctionToUnregisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, void* a_pArg)
{
    struct EvLoopInvokerEventsMonitor* const pMonitor = (struct EvLoopInvokerEventsMonitor*)a_pArg;
    EvLoopInvokerUnRegisterEventsMonitor(a_invokerHandle, pMonitor);
    return CPPUTILS_NULL;
}


static struct EvLoopInvokerEventsMonitor* RegisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, void* a_pData)
{
    struct EvLoopInvokerEventsMonitor* const pMonitor = (struct EvLoopInvokerEventsMonitor*)EvLoopInvokerCallFuncionBlocked(a_invokerHandle, &BlockedFunctionToRegisterMonitor, a_pData);
    return pMonitor;
}


static void UnregisterMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_invokerHandle, struct EvLoopInvokerEventsMonitor* a_monitor)
{
    EvLoopInvokerCallFuncionBlocked(a_invokerHandle, &BlockedFunctionToUnregisterMonitor, a_monitor);
}
