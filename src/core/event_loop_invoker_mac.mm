//
// repo:            private-input_monitor
// file:			privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>


#ifdef __APPLE__


#include <event_loop_invoker/event_loop_invoker_platform.h>
#include <event_loop_invoker/event_loop_invoker.h>
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <stddef.h>
#include <Foundation/Foundation.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C

struct EvLoopInvokerEventsMonitor{
    struct EvLoopInvokerEventsMonitor *prev, *next;
    EvLoopInvokerTypeEventMonitor   clbk;
    void*                           clbkData;
};


struct EvLoopInvokerHandle{
    NSOperationQueue*                   operationQueue;
    struct EvLoopInvokerEventsMonitor*  pFirstMonitor;
    ptrdiff_t                           inputArg;
    id                                  eventMonitor;
    CPPUTILS_BISTATE_FLAGS_UN(shouldRun, hasError)flags;
};

static inline void* EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData);


static inline bool EvLoopInvokerCallAllMonitorsInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, NSEvent* CPPUTILS_ARG_NN a_event){
    struct EvLoopInvokerEventsMonitor *pMonitorNext, *pMonitor = a_instance->pFirstMonitor;
    while(pMonitor){
        pMonitorNext = pMonitor->next;
        if((*(pMonitor->clbk))(a_instance,pMonitor->clbkData,a_event)){
            return true;
        }
        pMonitor = pMonitorNext;
    }  //  while(pMonitor){
    return false;
}


static inline int CreateEventMonitorIfNeededInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance){
    if(a_instance->eventMonitor){
        return 0;
    }

    const NSEventMask mask = ((a_instance->inputArg) > 0)
        ? (NSEventMask)(a_instance->inputArg)
        : NSEventMaskAny;

    a_instance->eventMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:mask
        handler:^(NSEvent* a_event){
            EvLoopInvokerCallAllMonitorsInline(a_instance,a_event);
        }];
    if(a_instance->eventMonitor){
        return 0;
    }
    return 1;
}


static inline void RemoveEventMonitorIfNeededInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance){
    if(a_instance->eventMonitor){
        if(a_instance->pFirstMonitor){
            return;
        }
        [NSEvent removeMonitor: a_instance->eventMonitor];
        a_instance->eventMonitor = CPPUTILS_NULL;
    }
}


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1,sizeof(struct EvLoopInvokerHandle));
    if(!pRetStr){
        CInternalLogError("Unable allocate buffer");
        return CPPUTILS_NULL;
    }

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    pRetStr->inputArg = (ptrdiff_t)a_inp;

    // Create an instance of NSOperationQueue with custom settings
    pRetStr->operationQueue = [[NSOperationQueue alloc] init];
    if(!(pRetStr->operationQueue)){
        CInternalLogError("Unable create an NSOperationQueue");
        free(pRetStr);
        return CPPUTILS_NULL;
    }

    pRetStr->operationQueue.maxConcurrentOperationCount = 1; // Allow 2 concurrent operations
    pRetStr->operationQueue.qualityOfService = NSQualityOfServiceUserInteractive; // Set quality of service to user interactive
    pRetStr->operationQueue.name = @"MyOperationQueue"; // Set a custom name for the queue
    pRetStr->operationQueue.underlyingQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0); // Set the underlying dispatch queue to a background queue
    pRetStr->operationQueue.suspended = NO; // Start the queue running

   return pRetStr;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance){
        a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
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


EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData)
{
    if(CreateEventMonitorIfNeededInline(a_instance)){
        return CPPUTILS_NULL;
    }

    struct EvLoopInvokerEventsMonitor* const pMonitor = (struct EvLoopInvokerEventsMonitor*)calloc(1,sizeof(struct EvLoopInvokerEventsMonitor));
    if(!pMonitor){
        return CPPUTILS_NULL;
    }

    pMonitor->clbk = a_fnc;
    pMonitor->clbkData = a_clbkData;
    pMonitor->prev = CPPUTILS_NULL;
    pMonitor->next = a_instance->pFirstMonitor;
    if(a_instance->pFirstMonitor){
        a_instance->pFirstMonitor->prev = pMonitor;
    }
    a_instance->pFirstMonitor = pMonitor;
    return pMonitor;
}


EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor)
{
    if(a_eventsMonitor){
        if(a_eventsMonitor->next){
            a_eventsMonitor->next->prev = a_eventsMonitor->prev;
        }
        if(a_eventsMonitor->prev){
            a_eventsMonitor->prev->next = a_eventsMonitor->next;
        }
        else{
            a_instance->pFirstMonitor = a_eventsMonitor->next;
        }
        free(a_eventsMonitor);
        RemoveEventMonitorIfNeededInline(a_instance);
    }  //  if(a_eventsMonitor){
}


EVLOOPINVK_EXPORT int EvLoopInvokerPtrToRequestCode(void* a_msg)
{
    NSEvent* const pEvent = EvLoopInvokerPtrToMsg(a_msg);
    const int evType = (int)([pEvent type]);
    return evType;
}


/*/////////////////////////////////////////////////////////////////////////////////*/

static inline void* EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData)
{
    struct EvLoopInvokerEventsMonitor *pMonitorNext, *pMonitor = a_instance->pFirstMonitor;
    while(pMonitor){
        pMonitorNext = pMonitor->next;
        free(pMonitor);
        pMonitor = pMonitorNext;
    }  //  while(pMonitor){
    a_instance->pFirstMonitor = CPPUTILS_NULL;

    RemoveEventMonitorIfNeededInline(a_instance);

    CPPUTILS_STATIC_CAST(void,a_pData);
    return CPPUTILS_NULL;
}


CPPUTILS_END_C


#endif  //  #ifdef __APPLE__
