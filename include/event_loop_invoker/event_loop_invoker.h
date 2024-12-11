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
#include <cinternal/undisable_compiler_warnings.h>

CPPUTILS_BEGIN_C

struct EvLoopInvokerHandle;
struct EvLoopInvokerEventsMonitor;
typedef void* (*EvLoopInvokerBlockedClbk)(struct EvLoopInvokerHandle*CPPUTILS_ARG_NN,void*);
typedef void (*EvLoopInvokerAsyncClbk)(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN,void*);
typedef bool (*EvLoopInvokerTypeEventMonitor)(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN,void*clbkData,void*msg);


#define EvLoopInvokerCreateHandle()     EvLoopInvokerCreateHandleEx(CPPUTILS_NULL)
EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT;
EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData);
EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor);


CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_H
