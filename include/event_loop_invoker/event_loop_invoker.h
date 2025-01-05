//
// repo:            event_loop_invoker
// file:			event_loop_invoker.h
// path:			include/event_loop_invoker/event_loop_invoker.h
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_H
#define EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_H

#include <event_loop_invoker/export_symbols.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdbool.h>
#include <stdint.h>
#include <cinternal/undisable_compiler_warnings.h>

CPPUTILS_BEGIN_C

struct EvLoopInvokerHandle;
struct EvLoopInvokerEventsMonitor;
typedef void* (*EvLoopInvokerBlockedClbk)(struct EvLoopInvokerHandle*CPPUTILS_ARG_NN,void*);
typedef void (*EvLoopInvokerAsyncClbk)(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN,void*);
typedef bool (*EvLoopInvokerTypeEventMonitor)(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN,void*clbkData,void*msg);

// ret<0 is error
enum EvLoopInvokerLoopReturn {
    EvLoopInvokerLoopReturnNone = 0,
    EvLoopInvokerLoopReturnError = 1,
    EvLoopInvokerLoopReturnInLoop = 2,
    EvLoopInvokerLoopReturnTimeout = 4,
    EvLoopInvokerLoopReturnQuit = 8
};


#define EvLoopInvokerCreateHandleForCurThr()                    EvLoopInvokerCreateHandleForCurThrEx(CPPUTILS_NULL)
#define EvLoopInvokerCallFuncionBlocked(_instance,_fnc,_data)   EvLoopInvokerCallFuncionBlockedEx(_instance,_fnc,_data,CPPUTILS_NULL)
#define EvLoopInvokerLoopEvLoopThrForever(_instance)            EvLoopInvokerLoopEvLoopThr(_instance,-1)
EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleForCurThrEx(const void* a_inp) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandleEvLoopThr(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void  EvLoopInvokerStopLoopAnyThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT int   EvLoopInvokerLoopEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlockedEx(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData, int* a_pnErrorCode) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT int   EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor) CPPUTILS_NOEXCEPT;

CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_H
