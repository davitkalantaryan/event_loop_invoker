//
// repo:            event_loop_invoker
// file:			event_loop_invoker_platform.h
// path:			include/event_loop_invoker/event_loop_invoker_platform.h
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H
#define EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H

#include <event_loop_invoker/event_loop_invoker.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdbool.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C

struct EvLoopInvokerEventsMonitor;


#ifdef _WIN32 
#elif defined(__linux__) || defined(__linux)

#include <cinternal/disable_compiler_warnings.h>
#include <xcb/xcb.h>
#include <cinternal/undisable_compiler_warnings.h>

typedef bool (*EvLoopInvokerTypeEventMonitor)(void*,xcb_generic_event_t*);

EVLOOPINVK_EXPORT xcb_connection_t* EvLoopInvokerCurrentXcbConnection(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance);
EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData);
EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor);

#elif defined(__APPLE__)
#else
# platform is not supported
#endif


CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H
