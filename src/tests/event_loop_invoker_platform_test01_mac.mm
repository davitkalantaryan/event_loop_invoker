//
// repo:            event_loop_invoker
// file:			main_event_loop_invoker_platform_test01.c
// path:			src/tests/main_event_loop_invoker_platform_test01.c
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef cinternal_gettid_needed
#define cinternal_gettid_needed
#endif

#include <event_loop_invoker/event_loop_invoker_platform.h>


CPPUTILS_BEGIN_C

#if defined(__APPLE__)

CPPUTILS_DLL_PRIVATE int EvLoopInvokerTestEventMonitorMac(void* a_msg)
{
    NSEvent* const pEvent = EvLoopInvokerPtrToMsg(a_msg);
    const int evType = (int)([pEvent type]);
    return evType;
}


#else
# platform is not supported
#endif


CPPUTILS_END_C
