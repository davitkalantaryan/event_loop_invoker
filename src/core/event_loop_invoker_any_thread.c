//
// repo:            private-input_monitor
// file:			event_loop_invoker_any_thread.c
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <event_loop_invoker/event_loop_invoker_any_thread.h>
#ifndef cinternal_unnamed_sema_wait_ms_needed
#define cinternal_unnamed_sema_wait_ms_needed
#endif
#include <cinternal/threading.h>
#include <cinternal/unnamed_semaphore.h>
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerHandleAnyThread {
    struct EvLoopInvokerHandle* instance;
    cinternal_thread_t          guiThread;
    cinternal_unnamed_sema_t    waitGuiThreadSema;
    const void*                 inputArg;
    int64_t                     durationMs;
    CPPUTILS_BISTATE_FLAGS_UN(
        shouldRun,
        isInLoop
    )flags;
    enum EvLoopInvokerLoopReturn    loopReturn;
    enum EvLoopInvokerLoopReturn    reserved01;
};

static cinternal_thread_ret_t CPPUTILS_THR_CALL EventLoopInvokerCallbacksThread(void* a_lpThreadParameter) CPPUTILS_NOEXCEPT;


EVLOOPINVK_EXPORT struct EvLoopInvokerHandleAnyThread* EvLoopInvokerCreateThreadAndHandleEx(const void* a_inp, int64_t a_durationMs) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandleAnyThread* const pRetStr = (struct EvLoopInvokerHandleAnyThread*)calloc(1, sizeof(struct EvLoopInvokerHandleAnyThread));
    if (!pRetStr) {
        return pRetStr;
    }

    if (cinternal_unnamed_sema_create(&(pRetStr->waitGuiThreadSema), 0)) {
        free(pRetStr);
        CInternalLogError("unable create semaphore");
        return CPPUTILS_NULL;
    }

    pRetStr->inputArg = a_inp;
    pRetStr->durationMs = a_durationMs;

    if (cinternal_thread_create(&(pRetStr->guiThread), &EventLoopInvokerCallbacksThread, pRetStr)) {
        cinternal_unnamed_sema_destroy(&(pRetStr->waitGuiThreadSema));
        free(pRetStr);
        CInternalLogError("unable create thread");
        return CPPUTILS_NULL;
    }

    cinternal_unnamed_sema_wait(&(pRetStr->waitGuiThreadSema));

    if (!(pRetStr->instance)) {
        cinternal_thread_ret_t threadRet;
        cinternal_thread_wait_and_clean(&(pRetStr->guiThread), &threadRet);
        cinternal_unnamed_sema_destroy(&(pRetStr->waitGuiThreadSema));
        free(pRetStr);
        CInternalLogError("error on event loop thread");
        return CPPUTILS_NULL;
    }

    return pRetStr;
}


EVLOOPINVK_EXPORT void EvLoopInvokerStopAndCleanHandle(struct EvLoopInvokerHandleAnyThread* a_instance) CPPUTILS_NOEXCEPT
{
    if (a_instance) {
        cinternal_thread_ret_t threadRet;
        a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        EvLoopInvokerStopLoopAnyThr(a_instance->instance);
        cinternal_unnamed_sema_post(&(a_instance->waitGuiThreadSema));
        cinternal_thread_wait_and_clean(&(a_instance->guiThread),&threadRet);
        cinternal_unnamed_sema_destroy(&(a_instance->waitGuiThreadSema));
        free(a_instance);
    }  //  if (a_instance) {
}


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerGetRawHandle(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return a_instance->instance;
}


EVLOOPINVK_EXPORT bool EvLoopInvokerIsInLoop(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return CPPUTILS_STATIC_CAST(bool,a_instance->flags.rd.isInLoop_true);
}


EVLOOPINVK_EXPORT enum EvLoopInvokerLoopReturn EvLoopInvokerLoopReturn(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return a_instance->loopReturn;
}


static cinternal_thread_ret_t CPPUTILS_THR_CALL EventLoopInvokerCallbacksThread(void* a_lpThreadParameter) CPPUTILS_NOEXCEPT
{
    int nLoopReturn;
    struct EvLoopInvokerHandleAnyThread* const pRetStr = (struct EvLoopInvokerHandleAnyThread*)a_lpThreadParameter;
    pRetStr->instance = EvLoopInvokerCreateHandleForCurThrEx(pRetStr->inputArg);

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;

    if (pRetStr->instance) {
        pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
        pRetStr->flags.wr.isInLoop = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
        pRetStr->loopReturn = EvLoopInvokerLoopReturnInLoop;
        cinternal_unnamed_sema_post(&(pRetStr->waitGuiThreadSema));

        nLoopReturn = EvLoopInvokerLoopEvLoopThr(pRetStr->instance, pRetStr->durationMs);
        pRetStr->flags.wr.isInLoop = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        if (nLoopReturn < 0) {
            pRetStr->loopReturn = EvLoopInvokerLoopReturnError;
        }
        else {
            pRetStr->loopReturn = CPPUTILS_STATIC_CAST(enum EvLoopInvokerLoopReturn, nLoopReturn);
        }
    }
    else {
        pRetStr->loopReturn = EvLoopInvokerLoopReturnError;
        cinternal_unnamed_sema_post(&(pRetStr->waitGuiThreadSema));
        cinternal_thread_exit_thread((cinternal_thread_ret_t)1);
    }

    while (pRetStr->flags.rd.shouldRun_true) {
        cinternal_unnamed_sema_wait(&(pRetStr->waitGuiThreadSema));
    }

    EvLoopInvokerCleanHandleEvLoopThr(pRetStr->instance);

    cinternal_thread_exit_thread((cinternal_thread_ret_t)0);
}


CPPUTILS_END_C
