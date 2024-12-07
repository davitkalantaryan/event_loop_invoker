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


CPPUTILS_BEGIN_C

struct EvLoopInvokerHandle;
typedef void* (*EvLoopInvokerBlockedClbk)(void*);
typedef void (*EvLoopInvokerAsyncClbk)(void*);


#define EvLoopInvokerCreateHandle()     EvLoopInvokerCreateHandleEx(CPPUTILS_NULL)
EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp);
EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance);
EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData);
EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData);


CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_H
