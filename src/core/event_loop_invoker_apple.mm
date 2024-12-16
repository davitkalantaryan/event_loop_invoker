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
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <stddef.h>
#include <Foundation/Foundation.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerHandle{
    struct EvLoopInvokerHandleBase      base;
    NSOperationQueue*                   operationQueue;
    ptrdiff_t                           inputArg;
    id                                  eventMonitor;
    CPPUTILS_BISTATE_FLAGS_UN(
        hasMainRunLoop
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


PrvEvLoopInvokerInline void RemoveEventMonitorInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance){
    [NSEvent removeMonitor: a_instance->eventMonitor];
    a_instance->eventMonitor = CPPUTILS_NULL;
}


PrvEvLoopInvokerInline void LoopedWaitInTheLoop(NSRunLoop* a_runLoop, int64_t a_timeMs){
    if(a_timeMs<0){
        //
    }
    else{
        const double waitTimeSec = ((double)a_timeMs)/1000.;
        NSDate*const futureDate = [NSDate dateWithTimeIntervalSinceNow:waitTimeSec];
        NSTimer*const timer = [NSTimer scheduledTimerWithTimeInterval:waitTimeSec
                repeats:NO
                block:^(NSTimer *tmr){
                    (void)tmr;
                }];
        [a_runLoop addTimer:timer forMode:NSRunLoopCommonModes];
        [a_runLoop runUntilDate:futureDate];
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

    NSRunLoop* const runLoop = [NSRunLoop currentRunLoop];
    if(runLoop && ([runLoop currentMode] == NSDefaultRunLoopMode) ){
        pRetStr->flags.wr.hasMainRunLoop = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    }
    else{
        pRetStr->flags.wr.hasMainRunLoop = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    }

    //pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
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
    
    EvLoopInvokerCallFuncionBlocked(pRetStr,&EventLoopInvokerInitInstanceInEventLoop,CPPUTILS_NULL);

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


EVLOOPINVK_EXPORT void EvLoopInvokerWaitForEventsMs(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_timeMs) CPPUTILS_NOEXCEPT
{
    @autoreleasepool {
        if([NSThread isMainThread]){
            NSRunLoop* const pMainRunLoop = [NSRunLoop mainRunLoop];
            LoopedWaitInTheLoop(pMainRunLoop,a_timeMs);
        }
        else{
            if(a_instance->flags.rd.hasMainRunLoop_false){
                __block const int64_t timeMs = a_timeMs;
                dispatch_async(dispatch_get_main_queue(), ^{
                    // Your code to be executed on the main thread goes here
                    NSRunLoop* const pMainRunLoop = [NSRunLoop mainRunLoop];
                    if ([pMainRunLoop currentMode] != NSDefaultRunLoopMode) {
                        // The run loop is not running, no concurency issue call it here
                        LoopedWaitInTheLoop(pMainRunLoop,timeMs);
                    }  //  if ([pMainRunLoop currentMode] != NSDefaultRunLoopMode) {
                });  //  dispatch_async(dispatch_get_main_queue(), ^{
            }  //  if(a_instance->flags.rd.hasMainRunLoop_false){
            else{
                NSRunLoop* const pCurRunLoop = [NSRunLoop currentRunLoop];
                LoopedWaitInTheLoop(pCurRunLoop,a_timeMs);
            }
        }
    }  //  @autoreleasepool {

}


/*/////////////////////////////////////////////////////////////////////////////////*/

static void* EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData) CPPUTILS_NOEXCEPT
{
    if(a_instance->eventMonitor){
        RemoveEventMonitorInline(a_instance);
    }
    EventLoopInvokerCleanInstanceInEventLoopInlineBase(&(a_instance->base));
    CPPUTILS_STATIC_CAST(void,a_pData);
    return CPPUTILS_NULL;
}


static void* EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, void* a_pData) CPPUTILS_NOEXCEPT
{
    EventLoopInvokerInitInstanceInEventLoopInlineBase(&(a_instance->base));
    CPPUTILS_STATIC_CAST(void,a_pData);
    return CPPUTILS_NULL;
}


CPPUTILS_END_C


#endif  //  #ifdef __APPLE__
