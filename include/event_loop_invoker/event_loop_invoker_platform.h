//
// repo:            event_loop_invoker
// file:			event_loop_invoker_platform.h
// path:			include/event_loop_invoker/event_loop_invoker_platform.h
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H
#define EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H

#include <event_loop_invoker/export_symbols.h>


CPPUTILS_BEGIN_C


#ifdef _WIN32

#include <cinternal/disable_compiler_warnings.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <cinternal/undisable_compiler_warnings.h>

#define EvLoopInvokerPtrToMsg(_ptr)         ((MSG*)(_ptr))
#define EvLoopInvokerPtrToRequestCode(_ptr) ( (int)(EvLoopInvokerPtrToMsg(_ptr)->message) )

#elif defined(__linux__) || defined(__linux)

#include <cinternal/disable_compiler_warnings.h>
#include <xcb/xcb.h>
#include <cinternal/undisable_compiler_warnings.h>

#define EvLoopInvokerPtrToMsg(_ptr)         ((xcb_generic_event_t*)(_ptr))
#define EvLoopInvokerPtrToRequestCode(_ptr) ( (int)(EvLoopInvokerPtrToMsg(_ptr)->response_type) )
#ifdef EvLoopInvoker_platform_specific_functions_needed
EVLOOPINVK_EXPORT xcb_connection_t* EvLoopInvokerCurrentXcbConnection(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance);
#endif

#elif defined(__APPLE__)

#ifdef __OBJC__
#include <cinternal/disable_compiler_warnings.h>
#import <AppKit/NSEvent.h>
#include <cinternal/undisable_compiler_warnings.h>
#endif
#define EvLoopInvokerPtrToMsg(_ptr)     ((NSEvent*)(_ptr))
#ifdef EvLoopInvoker_platform_specific_functions_needed
EVLOOPINVK_EXPORT int EvLoopInvokerPtrToRequestCode(void* a_msg);
#endif

#else
# platform is not supported
#endif


CPPUTILS_END_C



#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EVENT_LOOP_INVOKER_PLATFORM_H
