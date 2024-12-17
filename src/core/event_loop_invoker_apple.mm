//
// repo:            private-input_monitor
// file:			privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>


#ifdef __APPLE__


#include "event_loop_invoker_common.h"
#include <cinternal/logger.h>
#include <cinternal/bistateflags.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <Foundation/Foundation.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerHandle{
    struct EvLoopInvokerHandleBase      base;
    NSThread*                           evLoopThreaNsHandle;
    pthread_t                           evLoopThread;
    ptrdiff_t                           inputArg;
    dispatch_semaphore_t                sema;
    id                                  eventMonitor;
    CPPUTILS_BISTATE_FLAGS_UN(
        isOk
    )flags;
};

static void* EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData) CPPUTILS_NOEXCEPT;
static void* EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData) CPPUTILS_NOEXCEPT;


PrvEvLoopInvokerInline int CreateEventMonitorIfNeededInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance){
    if(a_instance->eventMonitor){
        return 0;
    }

    const NSEventMask mask = ((a_instance->inputArg) > 0)
        ? (NSEventMask)(a_instance->inputArg)
        : NSEventMaskAny;

    a_instance->eventMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:mask
        handler:^(NSEvent* a_event){
            EvLoopInvokerCallAllMonitorsInEventLoopInlineBase(&(a_instance->base),a_event);
        }];
    if(a_instance->eventMonitor){
        return 0;
    }
    return 1;
}


PrvEvLoopInvokerInline void EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance->eventMonitor){
        [NSEvent removeMonitor: a_instance->eventMonitor];
        a_instance->eventMonitor = CPPUTILS_NULL;
    }
    EventLoopInvokerCleanInstanceInEventLoopInlineBase(&(a_instance->base));
}


PrvEvLoopInvokerInline int EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    if(EventLoopInvokerInitInstanceInEventLoopInlineBase(&(a_instance->base))){
        return 1;
    }

    a_instance->evLoopThreaNsHandle = [NSThread currentThread];
    NSLog(@"Thread started: %@", a_instance->evLoopThreaNsHandle);
    if(!(a_instance->evLoopThreaNsHandle)){
        EventLoopInvokerCleanInstanceInEventLoop(a_instance);
        return 1;
    }

    return 0;
}


static void* EventLoopInvokerCallbacksThread(void* a_pData) CPPUTILS_NOEXCEPT;


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1,sizeof(struct EvLoopInvokerHandle));
    if(!pRetStr){
        CInternalLogError("Unable allocate buffer");
        return CPPUTILS_NULL;
    }

    pRetStr->sema = dispatch_semaphore_create(0);
    if(!(pRetStr->sema)){
        CInternalLogError("Unable to create semaphore");
        return CPPUTILS_NULL;
    }

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    //pRetStr->flags.wr.hasError = CPPUTILS_BISTATE_MAKE_BITS_TRUE;


    nRet = pthread_create(&(pRetStr->evLoopThread),CPPUTILS_NULL,&EventLoopInvokerCallbacksThread,pRetStr);
    if(nRet){
        CleanInstanceInline(pRetStr);
        CInternalLogError("Unable create a thread");
        return CPPUTILS_NULL;
    }

    return pRetStr;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance){
        //a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        EvLoopInvokerCallFuncionBlocked(a_instance,&EventLoopInvokerCleanInstanceInEventLoop,CPPUTILS_NULL);
        [a_instance->operationQueue release];
        free(a_instance);
    }
}


EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    void* pReturn = CPPUTILS_NULL;

    {
        __block struct EvLoopInvokerHandle* const pRetStr = a_instance;
        __block const EvLoopInvokerBlockedClbk fnc = a_fnc;
        __block void* const pData = a_pData;
        __block void**const ppReturn = &pReturn;
        NSBlockOperation *operation1 = [NSBlockOperation blockOperationWithBlock:^{
            *ppReturn = (*fnc)(pRetStr,pData);
        }];
        [pRetStr->operationQueue addOperations:@[operation1] waitUntilFinished:YES];
    }

    return pReturn;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    __block struct EvLoopInvokerHandle* const pRetStr = a_instance;
    __block const EvLoopInvokerAsyncClbk fnc = a_fnc;
    __block void* const pData = a_pData;
    NSBlockOperation *operation1 = [NSBlockOperation blockOperationWithBlock:^{
        (*fnc)(pRetStr,pData);
    }];
    [pRetStr->operationQueue addOperations:@[operation1] waitUntilFinished:NO];
}


EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData) CPPUTILS_NOEXCEPT
{
    if(CreateEventMonitorIfNeededInline(a_instance)){
        return CPPUTILS_NULL;
    }
    return EvLoopInvokerRegisterEventsMonitorEvLoopThrInlineBase(&(a_instance->base),a_fnc,a_clbkData);
}


EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor) CPPUTILS_NOEXCEPT
{
    if(EvLoopInvokerUnRegisterEventsMonitorEvLoopThrInlineBase(&(a_instance->base),a_eventsMonitor)){
        RemoveEventMonitorInline(a_instance);
    }
}


EVLOOPINVK_EXPORT int EvLoopInvokerPtrToRequestCode(void* a_msg)
{
    NSEvent* const pEvent = EvLoopInvokerPtrToMsg(a_msg);
    const int evType = (int)([pEvent type]);
    return evType;
}


/*/////////////////////////////////////////////////////////////////////////////////*/


static void* EventLoopInvokerCallbacksThread(void* a_pData) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)a_pData;

    nRet = EventLoopInvokerInitInstanceInEventLoop(pRetStr);
    if(nRet){
        pRetStr->flags.wr.isOk = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        dispatch_semaphore_signal(pRetStr->sema);
        pthread_exit((void*)((size_t)nRet));
    }

    pRetStr->flags.wr.isOk = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    dispatch_semaphore_signal(pRetStr->sema);

    [[NSRunLoop currentRunLoop] run];

    EventLoopInvokerCleanInstanceInEventLoop(pRetStr);

    pthread_exit(CPPUTILS_NULL);
}


CPPUTILS_END_C


#endif  //  #ifdef __APPLE__
