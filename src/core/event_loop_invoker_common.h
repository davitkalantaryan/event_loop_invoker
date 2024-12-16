//
// repo:            event_loop_invoker
// file:			event_loop_invoker_common.h
// path:			src/core/event_loop_invoker_common.h
// created on:		2024 Dec 16
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_SRC_CORE_EVENT_LOOP_INVOKER_COMMON_H
#define EVLOOPINVK_SRC_CORE_EVENT_LOOP_INVOKER_COMMON_H

#include <event_loop_invoker/export_symbols.h>
#include <event_loop_invoker/event_loop_invoker_platform.h>
#include <event_loop_invoker/event_loop_invoker.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


//#define PrvEvLoopInvokerInline
#ifndef PrvEvLoopInvokerInline
#define PrvEvLoopInvokerInline inline
#endif


struct EvLoopInvokerEventsMonitor{
    struct EvLoopInvokerEventsMonitor *prev, *next;
    EvLoopInvokerTypeEventMonitor   clbk;
    void*                           clbkData;
};


struct EvLoopInvokerHandleBase{
    struct EvLoopInvokerEventsMonitor*  pFirstMonitor;
    void*                               pReserved01;
};


static PrvEvLoopInvokerInline bool EvLoopInvokerCallAllMonitorsInEventLoopInlineBase(struct EvLoopInvokerHandleBase* CPPUTILS_ARG_NN a_instance, void* CPPUTILS_ARG_NN a_event) CPPUTILS_NOEXCEPT {
    struct EvLoopInvokerEventsMonitor *pMonitorNext, *pMonitor = a_instance->pFirstMonitor;
    while(pMonitor){
        pMonitorNext = pMonitor->next;
        if((*(pMonitor->clbk))((struct EvLoopInvokerHandle*)a_instance,pMonitor->clbkData,a_event)){
            return true;
        }
        pMonitor = pMonitorNext;
    }  //  while(pMonitor){
    return false;
}


static PrvEvLoopInvokerInline void EventLoopInvokerInitInstanceInEventLoopInlineBase(struct EvLoopInvokerHandleBase* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    a_instance->pFirstMonitor = CPPUTILS_NULL;
}


static PrvEvLoopInvokerInline void EventLoopInvokerCleanInstanceInEventLoopInlineBase(struct EvLoopInvokerHandleBase* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerEventsMonitor *pMonitorNext, *pMonitor = a_instance->pFirstMonitor;
    while(pMonitor){
        pMonitorNext = pMonitor->next;
        free(pMonitor);
        pMonitor = pMonitorNext;
    }  //  while(pMonitor){
    a_instance->pFirstMonitor = CPPUTILS_NULL;
}


static PrvEvLoopInvokerInline bool EvLoopInvokerUnRegisterEventsMonitorEvLoopThrInlineBase(struct EvLoopInvokerHandleBase* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor) CPPUTILS_NOEXCEPT
{
    if(a_eventsMonitor){
        if(a_eventsMonitor->next){
            a_eventsMonitor->next->prev = a_eventsMonitor->prev;
        }
        if(a_eventsMonitor->prev){
            a_eventsMonitor->prev->next = a_eventsMonitor->next;
        }
        else{
            a_instance->pFirstMonitor = a_eventsMonitor->next;
            free(a_eventsMonitor);
            if(a_instance->pFirstMonitor){
                return false;
            }
            return true;
        }
        free(a_eventsMonitor);
    }  //  if(a_eventsMonitor){
    return false;
}


static PrvEvLoopInvokerInline struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitorEvLoopThrInlineBase(struct EvLoopInvokerHandleBase* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerEventsMonitor* const pMonitor = (struct EvLoopInvokerEventsMonitor*)calloc(1,sizeof(struct EvLoopInvokerEventsMonitor));
    if(!pMonitor){
        return CPPUTILS_NULL;
    }

    pMonitor->clbk = a_fnc;
    pMonitor->clbkData = a_clbkData;
    pMonitor->prev = CPPUTILS_NULL;
    pMonitor->next = a_instance->pFirstMonitor;
    if(a_instance->pFirstMonitor){
        a_instance->pFirstMonitor->prev = pMonitor;
    }
    a_instance->pFirstMonitor = pMonitor;
    return pMonitor;
}


CPPUTILS_END_C


#endif  //  #ifndef EVLOOPINVK_SRC_CORE_EVENT_LOOP_INVOKER_COMMON_H
