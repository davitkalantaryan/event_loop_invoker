//
// repo:            event_loop_invoker
// file:			event_loop_invoker_any_thread.h
// path:			include/event_loop_invoker/event_loop_invoker_any_thread.h
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_ANY_THREAD_H
#define EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_ANY_THREAD_H

#include <event_loop_invoker/export_symbols.h>
#include <event_loop_invoker/event_loop_invoker.h>
#include <stdbool.h>


CPPUTILS_BEGIN_C

struct EvLoopInvokerHandleAnyThread;


#define EvLoopInvokerCreateThreadAndHandleTm(_timeMs)           EvLoopInvokerCreateThreadAndHandleEx(CPPUTILS_NULL,(_timeMs))
#define EvLoopInvokerCreateThreadAndHandle()                    EvLoopInvokerCreateThreadAndHandleTm(-1)
EVLOOPINVK_EXPORT struct EvLoopInvokerHandleAnyThread* EvLoopInvokerCreateThreadAndHandleEx(const void* a_inp, int64_t a_durationMs) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void  EvLoopInvokerStopAndCleanHandle(struct EvLoopInvokerHandleAnyThread* a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerGetRawHandle(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT bool EvLoopInvokerIsInLoop(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT enum EvLoopInvokerLoopReturn EvLoopInvokerLoopReturn(struct EvLoopInvokerHandleAnyThread* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;


CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_ANY_THREAD_H
