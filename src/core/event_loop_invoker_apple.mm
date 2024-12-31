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
#include <time.h>
#include <dispatch/dispatch.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerInvokeBlockedData{
    EvLoopInvokerBlockedClbk    fnc;
    dispatch_semaphore_t        sema;
    void*                       arg2;
    void*                       pRet;
};

struct EvLoopInvokerInvokeAsyncData{
    EvLoopInvokerAsyncClbk      fnc;
    CFRunLoopSourceRef          cFsourceAsync;
    struct EvLoopInvokerHandle* arg1;
    void*                       arg2;
};


struct EvLoopInvokerHandle{
    struct EvLoopInvokerHandleBase          base;
    CFRunLoopRef                            evLoopThreaCfLoopHandle;
    CFRunLoopSourceRef                      cFsourceBlocked;
    ptrdiff_t                               inputArg;
    id                                      eventMonitor;
    struct EvLoopInvokerInvokeBlockedData   blockedClbkData;
    CPPUTILS_BISTATE_FLAGS_UN(
        shouldRun
    )flags;
};


static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static int  EvLoopInvokerLoopWithTimeoutEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT;


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


static void RunLoopSourcePerformBlocked(void* CPPUTILS_ARG_NN a_pData) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)a_pData;
    pRetStr->blockedClbkData.pRet = (*(pRetStr->blockedClbkData.fnc))(pRetStr,pRetStr->blockedClbkData.arg2);
    dispatch_semaphore_signal(pRetStr->blockedClbkData.sema);
}


static void RunLoopSourcePerformAsync(void* CPPUTILS_ARG_NN a_pData) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerInvokeAsyncData* const pData = (struct EvLoopInvokerInvokeAsyncData*)a_pData;
    (*(pData->fnc))(pData->arg1,pData->arg2);
    CFRunLoopRemoveSource(pData->arg1->evLoopThreaCfLoopHandle, pData->cFsourceAsync, kCFRunLoopDefaultMode);
    CFRelease(pData->cFsourceAsync);
    free(pData);
}


PrvEvLoopInvokerInline void EventLoopInvokerCleanInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance->eventMonitor){
        [NSEvent removeMonitor: a_instance->eventMonitor];
        a_instance->eventMonitor = CPPUTILS_NULL;
    }

    if(a_instance->cFsourceBlocked){
        CFRunLoopRemoveSource(a_instance->evLoopThreaCfLoopHandle, a_instance->cFsourceBlocked, kCFRunLoopDefaultMode);
        CFRelease(a_instance->cFsourceBlocked);
        a_instance->cFsourceBlocked = CPPUTILS_NULL;
    }

    a_instance->evLoopThreaCfLoopHandle = CPPUTILS_NULL;

    EventLoopInvokerCleanInstanceInEventLoopInlineBase(&(a_instance->base));
}


PrvEvLoopInvokerInline int EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    CFRunLoopSourceContext context;

    if(EventLoopInvokerInitInstanceInEventLoopInlineBase(&(a_instance->base))){
        return 1;
    }

    a_instance->evLoopThreaCfLoopHandle = CFRunLoopGetCurrent();
    NSLog(@"Loop handle: %@", a_instance->evLoopThreaCfLoopHandle);
    if(!(a_instance->evLoopThreaCfLoopHandle)){
        CInternalLogError("CFRunLoopGetCurrent failed");
        EventLoopInvokerCleanInstanceInEventLoop(a_instance);
        return 1;
    }

    memset(&context,0,sizeof(CFRunLoopSourceContext));
    context.info = a_instance;
    context.perform = &RunLoopSourcePerformBlocked;
    a_instance->cFsourceBlocked = CFRunLoopSourceCreate(CPPUTILS_NULL, 0, &context);
    if(!(a_instance->cFsourceBlocked)){
        CInternalLogError("CFRunLoopGetCurrent failed");
        EventLoopInvokerCleanInstanceInEventLoop(a_instance);
        return 1;
    }
    CFRunLoopAddSource(a_instance->evLoopThreaCfLoopHandle,a_instance->cFsourceBlocked,kCFRunLoopDefaultMode);

    return 0;
}


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleForCurThrEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1,sizeof(struct EvLoopInvokerHandle));
    if(!pRetStr){
        CInternalLogError("Unable allocate buffer");
        return CPPUTILS_NULL;
    }

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    pRetStr->inputArg = (ptrdiff_t)a_inp;

    nRet = EventLoopInvokerInitInstanceInEventLoop(pRetStr);
    if(nRet){
        free(pRetStr);
        CInternalLogError("Error during initialization");
        return CPPUTILS_NULL;
    }

    return pRetStr;
}


EVLOOPINVK_EXPORT void EvLoopInvokerCleanHandleEvLoopThr(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance){
        EventLoopInvokerCleanInstanceInEventLoop(a_instance);
        free(a_instance);
    }
}


EVLOOPINVK_EXPORT void EvLoopInvokerStopLoopAnyThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    CFRunLoopStop(a_instance->evLoopThreaCfLoopHandle); // Stop the run loop
}


EVLOOPINVK_EXPORT int EvLoopInvokerLoopEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT
{
    if (a_durationMs < 0) {
        EventLoopInvokerInfiniteEventLoop(a_instance);
        return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnQuit);
    }  //  if (a_durationMs < 0) {
    return EvLoopInvokerLoopWithTimeoutEvLoopThr(a_instance, a_durationMs);
}


EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlockedEx(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData, int* a_pnErrorCode) CPPUTILS_NOEXCEPT
{
    const int64_t curThrId = CinternalGetCurrentTid();
    if(curThrId==(a_instance->base.evLoopTid)){
        if(a_pnErrorCode){
            *a_pnErrorCode = 0;
        }
        return (*a_fnc)(a_instance,a_pData);
    }
    a_instance->blockedClbkData.fnc = a_fnc;
    a_instance->blockedClbkData.sema = dispatch_semaphore_create(0);
    if(!(a_instance->blockedClbkData.sema)){
        if(a_pnErrorCode){
            *a_pnErrorCode = 1;
        }
        return CPPUTILS_NULL;
    }
    a_instance->blockedClbkData.arg2 = a_pData;
    CFRunLoopSourceSignal(a_instance->cFsourceBlocked);
    CFRunLoopWakeUp(a_instance->evLoopThreaCfLoopHandle);
    dispatch_semaphore_wait(a_instance->blockedClbkData.sema,DISPATCH_TIME_FOREVER);
    dispatch_release(a_instance->blockedClbkData.sema);
    a_instance->blockedClbkData.sema = (dispatch_semaphore_t)0;
    if(a_pnErrorCode){
        *a_pnErrorCode = 0;
    }
    return a_instance->blockedClbkData.pRet;
}


EVLOOPINVK_EXPORT int EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    CFRunLoopSourceContext context ;
    struct EvLoopInvokerInvokeAsyncData* const pData = (struct EvLoopInvokerInvokeAsyncData*)calloc(1,sizeof(struct EvLoopInvokerInvokeAsyncData));
    if(!pData){
        return 1;
    }
    pData->fnc = a_fnc;
    pData->arg1 = a_instance;
    pData->arg2 = a_pData;

    memset(&context,0,sizeof(CFRunLoopSourceContext));
    context.info = pData;
    context.perform = &RunLoopSourcePerformAsync;
    pData->cFsourceAsync = CFRunLoopSourceCreate(CPPUTILS_NULL, 0, &context);
    if(!(pData->cFsourceAsync)){
        free(pData);
        CInternalLogError("CFRunLoopGetCurrent failed");
        return 1;
    }
    CFRunLoopAddSource(a_instance->evLoopThreaCfLoopHandle,pData->cFsourceAsync,kCFRunLoopDefaultMode);
    CFRunLoopSourceSignal(pData->cFsourceAsync);
    CFRunLoopWakeUp(a_instance->evLoopThreaCfLoopHandle);
    return 0;
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
        [NSEvent removeMonitor: a_instance->eventMonitor];
        a_instance->eventMonitor = CPPUTILS_NULL;
    }
}


EVLOOPINVK_EXPORT int EvLoopInvokerPtrToRequestCode(void* a_msg)
{
    NSEvent* const pEvent = EvLoopInvokerPtrToMsg(a_msg);
    const int evType = (int)([pEvent type]);
    return evType;
}


/*/////////////////////////////////////////////////////////////////////////////////*/

static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    while(a_instance->flags.rd.shouldRun_true){
        CFRunLoopRun();
    }
}


static int  EvLoopInvokerLoopWithTimeoutEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT
{
    time_t currentTime, startTime;
    int64_t durationRemaining = a_durationMs;
    CFTimeInterval timeToLoop;
    CFRunLoopRunResult loopRes;
    const CFAbsoluteTime distantFuture = CFAbsoluteTimeGetCurrent() + 1e10; // 10 billion seconds in the future
    CFRunLoopTimerContext timerContext = {0, NULL, NULL, NULL, NULL};
    CFRunLoopTimerRef timer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,         // Allocator
        distantFuture,               // Fire date
        0,                           // Interval (0 = one-time timer)
        0,                           // Flags
        0,                           // Order
        NULL,                        // Callback function (NULL since it won't fire)
        &timerContext                // Context
    );

    // Add the timer to the run loop
    CFRunLoopAddTimer(a_instance->evLoopThreaCfLoopHandle, timer, kCFRunLoopDefaultMode);

    startTime = time(&startTime);

    do {
        timeToLoop = CPPUTILS_STATIC_CAST(CFTimeInterval,durationRemaining) / CPPUTILS_STATIC_CAST(CFTimeInterval,1000) ;
        loopRes = CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeToLoop, false);
        switch(loopRes){
        case kCFRunLoopRunTimedOut:
            return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnTimeout);
        default:
            break;
        }  //  switch(loopRes){

        currentTime = time(&currentTime);
        durationRemaining = a_durationMs - CPPUTILS_STATIC_CAST(int64_t, currentTime-startTime);

    } while ((durationRemaining >= 0)&&(a_instance->flags.rd.shouldRun_true));

    CFRelease(timer);

    if (a_instance->flags.rd.shouldRun_false) {
        return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnQuit);
    }

    return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnTimeout);
}


CPPUTILS_END_C


#endif  //  #ifdef __APPLE__
