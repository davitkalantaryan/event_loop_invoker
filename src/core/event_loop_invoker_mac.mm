//
// repo:            private-input_monitor
// file:			privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>


#ifdef CPPUTILS_OS_MACOS


#include <event_loop_invoker/event_loop_invoker.h>
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <stdlib.h>
#include <Foundation/Foundation.h>


CPPUTILS_BEGIN_C


struct EvLoopInvokerHandle{
    NSOperationQueue*                   operationQueue;
    CPPUTILS_BISTATE_FLAGS_UN(shouldRun, hasError)flags;
};


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp)
{
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1,sizeof(struct EvLoopInvokerHandle));
    if(!pRetStr){
        CInternalLogError("Unable allocate buffer");
        return CPPUTILS_NULL;
    }

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;

    (void)a_inp;

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


EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance)
{
    if(a_instance){
        a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        //AppAndBrowsMonRunClbkOnGuiThreadInline(a_instance,a_instance,&ClbkForDataCleaning);
        [a_instance->operationQueue release];
        free(a_instance);
    }
}


EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData)
{
    void* pReturn = CPPUTILS_NULL;

    {
        __block struct EvLoopInvokerHandle* const pRetStr = a_instance;
        __block void**const ppReturn = &pReturn;
        NSBlockOperation *operation1 = [NSBlockOperation blockOperationWithBlock:^{
            *ppReturn = (*a_fnc)(a_pData);
        }];
        [pRetStr->operationQueue addOperations:@[operation1] waitUntilFinished:YES];
    }

    return pReturn;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData)
{
    __block struct EvLoopInvokerHandle* const pRetStr = a_instance;
    NSBlockOperation *operation1 = [NSBlockOperation blockOperationWithBlock:^{
        (*a_fnc)(a_pData);
    }];
    [pRetStr->operationQueue addOperations:@[operation1] waitUntilFinished:NO];
}


CPPUTILS_END_C


#endif  //  #ifdef CPPUTILS_OS_MACOS
