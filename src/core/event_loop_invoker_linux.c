//
// repo:            private-input_monitor
// file:			privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>


#if defined(__linux__) || defined(__linux)

#ifndef cinternal_gettid_needed
#define cinternal_gettid_needed
#endif

#include "event_loop_invoker_common.h"
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <cinternal/gettid.h>
#include <cinternal/signals.h>
#include <cinternal/disable_compiler_warnings.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


#define EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK     1
#define EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK       2


struct EvLoopInvokerBlockedCallData{
    sem_t                               sema;
    EvLoopInvokerBlockedClbk            fnc;
    void*                               pInOut;
};

struct EvLoopInvokerAsyncCallData{
    EvLoopInvokerAsyncClbk              fnc;
    void*                               pIn;
};

struct EvLoopInvokerCallData{
    uint8_t type;
    union{
        struct EvLoopInvokerBlockedCallData*    blockedCall_p;
        struct EvLoopInvokerAsyncCallData       asyncCall;
    };
};


struct EvLoopInvokerHandle{
    struct EvLoopInvokerHandleBase      base;
    char*                               displayName;
    pthread_t                           monitor_thread;
    sem_t                               sema;
    int                                 screen_default_nbr;
    xcb_connection_t*                   connection;
    xcb_screen_t*                       default_screen;
    xcb_window_t                        msg_window;
    
    CPPUTILS_BISTATE_FLAGS_UN(shouldRun, hasError, semaCreated)flags;
};


static void* EventLoopInvokerCallbacksThread(void* a_pData) CPPUTILS_NOEXCEPT;


static PrvEvLoopInvokerInline void CleanInstanceInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT {
    if(a_instance->flags.rd.semaCreated_true){
        sem_destroy(&(a_instance->sema));
    }
    free(a_instance->displayName);
    free(a_instance);
}


static PrvEvLoopInvokerInline void EventLoopInvokerCleanInstanceInEventLoopInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{    
    if(a_instance->msg_window){
        xcb_destroy_window(a_instance->connection, a_instance->msg_window);
        a_instance->msg_window = 0;
    }
    
    if(a_instance->connection){
        xcb_disconnect(a_instance->connection);
        a_instance->connection = CPPUTILS_NULL;
    }
    
    EventLoopInvokerCleanInstanceInEventLoopInlineBase(&(a_instance->base));
}


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1,sizeof(struct EvLoopInvokerHandle));
    if(!pRetStr){
        CInternalLogError("Unable to create memory");
        return CPPUTILS_NULL;
    }
    
    if(a_inp){
        pRetStr->displayName = strdup((const char*)a_inp);
        if(!pRetStr->displayName){
            free(pRetStr);
            CInternalLogError("Unable to create memory for display name");
            return CPPUTILS_NULL;
        }
    }
    
    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    
    nRet = sem_init( &(pRetStr->sema), 0, 0);  // first 0 that not shared between processes, second 0 is the initial count
    if(nRet){
        CleanInstanceInline(pRetStr);
        CInternalLogError("Unable create a sema");
        return CPPUTILS_NULL;
    }
    pRetStr->flags.wr.semaCreated = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
    
    nRet = pthread_create(&(pRetStr->monitor_thread),CPPUTILS_NULL,&EventLoopInvokerCallbacksThread,pRetStr);
    if(nRet){
        CleanInstanceInline(pRetStr);
        CInternalLogError("Unable create a thread");
        return CPPUTILS_NULL;
    }
    
    sem_wait( &(pRetStr->sema) );
    sem_destroy( &(pRetStr->sema) );
    pRetStr->flags.wr.semaCreated = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    
    if(pRetStr->flags.rd.hasError_true){
        CleanInstanceInline(pRetStr);
        CInternalLogError("Error in the GUI thread");
        return CPPUTILS_NULL;
    }
    
    return pRetStr;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance){
        void* pReturn;
        xcb_client_message_event_t clntMsg;
        
        a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        memset(&clntMsg, 0, sizeof(clntMsg));
        
        clntMsg.response_type = XCB_CLIENT_MESSAGE;
        clntMsg.format = 32; // 32-bit data format
        clntMsg.window = a_instance->msg_window;
        clntMsg.type = XCB_ATOM_WM_COMMAND;
        
        xcb_send_event(a_instance->connection, 0, a_instance->msg_window, XCB_EVENT_MASK_NO_EVENT, (char *)&clntMsg);
        xcb_flush(a_instance->connection);
        
        pthread_join(a_instance->monitor_thread,&pReturn);
        
        CleanInstanceInline(a_instance);
        
    }  //  if(a_instance){
}



EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerCallData* pCalldata;
    struct EvLoopInvokerBlockedCallData blockedCallData;
    xcb_client_message_event_t clntMsg;
    
    memset(&clntMsg, 0, sizeof(clntMsg));
    clntMsg.response_type = XCB_CLIENT_MESSAGE;
    clntMsg.format = 32; // 32-bit data format
    clntMsg.window = a_instance->msg_window;
    clntMsg.type = XCB_ATOM_WM_COMMAND;
    pCalldata = (struct EvLoopInvokerCallData*)(&(clntMsg.data));
    
    pCalldata->type = EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK;
    pCalldata->blockedCall_p = &blockedCallData;
    blockedCallData.fnc = a_fnc;
    blockedCallData.pInOut = a_pData;
    nRet = sem_init( &(blockedCallData.sema), 0, 0);  // first 0 that not shared between processes, second 0 is the initial count
    if(nRet){
        CInternalLogError("Unable create a sema");
        return CPPUTILS_NULL;
    }
    
    xcb_send_event(a_instance->connection, 0, a_instance->msg_window, XCB_EVENT_MASK_NO_EVENT, (char *)&clntMsg);
    xcb_flush(a_instance->connection);
    
    sem_wait( &(blockedCallData.sema) );
    sem_destroy( &(blockedCallData.sema) );
    return blockedCallData.pInOut;
}



EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerCallData* pCalldata;
    xcb_client_message_event_t clntMsg;
    
    memset(&clntMsg, 0, sizeof(clntMsg));
    clntMsg.response_type = XCB_CLIENT_MESSAGE;
    clntMsg.format = 32; // 32-bit data format
    clntMsg.window = a_instance->msg_window;
    clntMsg.type = XCB_ATOM_WM_COMMAND;
    pCalldata = (struct EvLoopInvokerCallData*)(&(clntMsg.data));
    
    pCalldata->type = EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK;
    pCalldata->asyncCall.fnc = a_fnc;
    pCalldata->asyncCall.pIn = a_pData;
    
    xcb_send_event(a_instance->connection, 0, a_instance->msg_window, XCB_EVENT_MASK_NO_EVENT, (char *)&clntMsg);
    xcb_flush(a_instance->connection);
}


/*/// platform specific api  ///*/

EVLOOPINVK_EXPORT xcb_connection_t* EvLoopInvokerCurrentXConnectionEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return a_instance->connection;
}


EVLOOPINVK_EXPORT xcb_screen_t* EvLoopInvokerCurrentXDefaultDisplayEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    return a_instance->default_screen;
}


EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData) CPPUTILS_NOEXCEPT
{
    return EvLoopInvokerRegisterEventsMonitorEvLoopThrInlineBase(&(a_instance->base),a_fnc,a_clbkData);
}


EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitorEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor) CPPUTILS_NOEXCEPT
{
    EvLoopInvokerUnRegisterEventsMonitorEvLoopThrInlineBase(&(a_instance->base),a_eventsMonitor);
}


EVLOOPINVK_EXPORT void EvLoopInvokerWaitForEventsMs(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_timeMs) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void,a_instance);
    CinternalSleepInterruptableMs(a_timeMs);
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////*/

static int EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;


static void* EventLoopInvokerCallbacksThread(void* a_pData) CPPUTILS_NOEXCEPT
{
    int nRet;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)a_pData;

    nRet = EventLoopInvokerInitInstanceInEventLoop(pRetStr);
    if(nRet){
        pRetStr->flags.wr.hasError = CPPUTILS_BISTATE_MAKE_BITS_TRUE;
        sem_post( &(pRetStr->sema) );
        pthread_exit((void*)((size_t)nRet));
    }
    
    pRetStr->flags.wr.hasError = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    sem_post( &(pRetStr->sema) );

    EventLoopInvokerInfiniteEventLoop(pRetStr);
    EventLoopInvokerCleanInstanceInEventLoopInline(pRetStr);

    pthread_exit(CPPUTILS_NULL);
}


static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    xcb_generic_event_t* event;

    while ( a_instance->flags.rd.shouldRun_true ) {
        event = xcb_wait_for_event(a_instance->connection);
        
        if(!EvLoopInvokerCallAllMonitorsInEventLoopInlineBase(&(a_instance->base),event)){
            switch (event->response_type & ~0x80) {
            case XCB_CLIENT_MESSAGE:{
                xcb_client_message_event_t* const clntMsg_p = (xcb_client_message_event_t *)event;
                if((clntMsg_p->window)==(a_instance->msg_window)){
                    void* pRet;
                    struct EvLoopInvokerCallData* const pCalldata = (struct EvLoopInvokerCallData*)(&(clntMsg_p->data));
                    switch(pCalldata->type){
                    case EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK:
                        pRet = (*(pCalldata->blockedCall_p->fnc))(a_instance,pCalldata->blockedCall_p->pInOut);
                        pCalldata->blockedCall_p->pInOut = pRet;
                        sem_post( &(pCalldata->blockedCall_p->sema) );
                        break;
                    case EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK:
                        (*(pCalldata->asyncCall.fnc))(a_instance,pCalldata->asyncCall.pIn);
                        break;
                    default:
                        break;
                    }  //  switch(clntMsg->data.data8[0]){
                }  //  f((clntMsg_p->window)==(a_instance->msg_window)){
            }break;  //  case XCB_CLIENT_MESSAGE:{
            default:
                break;
            }  //  switch (event->response_type & ~0x80) {
        }  //  if(!EvLoopInvokerCallAllMonitorsInline(a_instance,event)){
                
        free(event);
        
    }  //  while ( a_instance->flags.rd.shouldRun_true ) {
}


static PrvEvLoopInvokerInline xcb_screen_t *screen_of_display (xcb_connection_t* a_connection, int a_screen) CPPUTILS_NOEXCEPT {
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator (xcb_get_setup (a_connection));
    for (; iter.rem; --a_screen, xcb_screen_next (&iter))
      if (a_screen == 0)
        return iter.data;

    return NULL;
}


static PrvEvLoopInvokerInline xcb_window_t create_message_window(xcb_connection_t *connection, xcb_screen_t *screen) CPPUTILS_NOEXCEPT {
    xcb_window_t window = xcb_generate_id(connection);
    uint32_t value_mask = XCB_CW_EVENT_MASK;
    uint32_t value_list[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };

    xcb_create_window(connection,
                      XCB_COPY_FROM_PARENT, // depth
                      window,              // window ID
                      screen->root,        // parent window
                      0, 0,                // x, y position
                      1, 1,                // width, height (minimal size)
                      0,                   // border width
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, // visual
                      value_mask, value_list);

    // Do not map the window; it's invisible.
    return window;
}


static int EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    EventLoopInvokerInitInstanceInEventLoopInlineBase(&(a_instance->base));
    
    a_instance->connection = xcb_connect( a_instance->displayName, &(a_instance->screen_default_nbr));
    if (xcb_connection_has_error(a_instance->connection)){
        xcb_disconnect(a_instance->connection);
        a_instance->connection = CPPUTILS_NULL;
        CInternalLogError(" xcb_connection_has_error ");
        return 1;
    }
    
    a_instance->default_screen = screen_of_display (a_instance->connection, a_instance->screen_default_nbr);
    if(!(a_instance->default_screen)){
        xcb_disconnect(a_instance->connection);
        a_instance->connection = CPPUTILS_NULL;
        CInternalLogError(" No root screen ");
        return 1;
    }
    
    a_instance->msg_window = create_message_window(a_instance->connection,a_instance->default_screen);
    if(!a_instance->msg_window){
        EventLoopInvokerCleanInstanceInEventLoopInline(a_instance);
        return 1;
    }
        
    return 0;
}


CPPUTILS_END_C


#endif  //  #if defined(__linux__) || defined(__linux)
