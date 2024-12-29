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
#include <cinternal/logger.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerHandleAnyThread {
    struct EvLoopInvokerHandle* instance;
    cinternal_thread_t          guiThread;
    cinternal_unnamed_sema_t    waitGuiThreadSema;
    const void*                 inputArg;
    int64_t                     durationMs;
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
    cinternal_unnamed_sema_destroy(&(pRetStr->waitGuiThreadSema));

    if (!(pRetStr->instance)) {
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
        EvLoopInvokerStopLoopAnyThr(a_instance->instance);
        cinternal_thread_wait_and_clean(&(a_instance->guiThread),&threadRet);
        free(a_instance);
    }  //  if (a_instance) {
}


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerGetRawHandle(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return a_instance->instance;
}


static cinternal_thread_ret_t CPPUTILS_THR_CALL EventLoopInvokerCallbacksThread(void* a_lpThreadParameter) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandleAnyThread* const pRetStr = (struct EvLoopInvokerHandleAnyThread*)a_lpThreadParameter;
    pRetStr->instance = EvLoopInvokerCreateHandleForCurThrEx(pRetStr->inputArg);

    if (!(pRetStr->instance)) {
        cinternal_unnamed_sema_post(&(pRetStr->waitGuiThreadSema));
        cinternal_thread_exit_thread(1);
    }

    cinternal_unnamed_sema_post(&(pRetStr->waitGuiThreadSema));

    EvLoopInvokerLoopEvLoopThr(pRetStr->instance,pRetStr->durationMs);

    EvLoopInvokerCleanHandleEvLoopThr(pRetStr->instance);

    cinternal_thread_exit_thread(0);
}


CPPUTILS_END_C
