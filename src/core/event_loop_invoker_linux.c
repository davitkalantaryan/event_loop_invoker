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
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
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
    int                                 screen_default_nbr;
    xcb_connection_t*                   connection;
    xcb_screen_t*                       default_screen;
    xcb_window_t                        msg_window;
    
    CPPUTILS_BISTATE_FLAGS_UN(
        shouldRun
    )flags;
};


static int  EventLoopInvokerInitInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static int  EvLoopInvokerLoopWithTimeoutEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT;


PrvEvLoopInvokerInline void EventLoopInvokerCleanInstanceInEventLoopInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
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


PrvEvLoopInvokerInline void CleanInstanceInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT {
    EventLoopInvokerCleanInstanceInEventLoopInline(a_instance);
    free(a_instance->displayName);
    free(a_instance);
}


PrvEvLoopInvokerInline void EventLoopInvokerHandleSingleEventInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, xcb_generic_event_t* CPPUTILS_ARG_NN a_event) CPPUTILS_NOEXCEPT{
    if(!EvLoopInvokerCallAllMonitorsInEventLoopInlineBase(&(a_instance->base),a_event)){
        switch (a_event->response_type & ~0x80) {
        case XCB_CLIENT_MESSAGE:{
            xcb_client_message_event_t* const clntMsg_p = (xcb_client_message_event_t *)a_event;
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
}


PrvEvLoopInvokerInline xcb_screen_t *screen_of_display (xcb_connection_t* a_connection, int a_screen) CPPUTILS_NOEXCEPT {
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator (xcb_get_setup (a_connection));
    for (; iter.rem; --a_screen, xcb_screen_next (&iter))
      if (a_screen == 0)
        return iter.data;

    return NULL;
}


PrvEvLoopInvokerInline xcb_window_t create_message_window(xcb_connection_t *connection, xcb_screen_t *screen) CPPUTILS_NOEXCEPT {
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


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleForCurThrEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
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
    
    if(EventLoopInvokerInitInstanceInEventLoop(pRetStr)){
        CleanInstanceInline(pRetStr);
        return CPPUTILS_NULL;
    }
        
    return pRetStr;
}


EVLOOPINVK_EXPORT void EvLoopInvokerCleanHandleEvLoopThr(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if(a_instance){
        CleanInstanceInline(a_instance);        
    }  //  if(a_instance){
}


EVLOOPINVK_EXPORT void EvLoopInvokerStopLoopAnyThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    xcb_client_message_event_t clntMsg;
    
    a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    memset(&clntMsg, 0, sizeof(clntMsg));
    
    clntMsg.response_type = XCB_CLIENT_MESSAGE;
    clntMsg.format = 32; // 32-bit data format
    clntMsg.window = a_instance->msg_window;
    clntMsg.type = XCB_ATOM_WM_COMMAND;
    
    xcb_send_event(a_instance->connection, 0, a_instance->msg_window, XCB_EVENT_MASK_NO_EVENT, (char *)&clntMsg);
    xcb_flush(a_instance->connection);
}


EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlockedEx(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData, int* a_pnErrorCode) CPPUTILS_NOEXCEPT
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
        if(a_pnErrorCode){
            *a_pnErrorCode = 1;
        }
        return CPPUTILS_NULL;
    }
    
    xcb_send_event(a_instance->connection, 0, a_instance->msg_window, XCB_EVENT_MASK_NO_EVENT, (char *)&clntMsg);
    xcb_flush(a_instance->connection);
    
    sem_wait( &(blockedCallData.sema) );
    sem_destroy( &(blockedCallData.sema) );
    
    if(a_pnErrorCode){
        *a_pnErrorCode = 0;
    }
    
    return blockedCallData.pInOut;
}



EVLOOPINVK_EXPORT int  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
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
    return 0;
}


EVLOOPINVK_EXPORT int EvLoopInvokerLoopEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT
{
    if (a_durationMs < 0) {
        EventLoopInvokerInfiniteEventLoop(a_instance);
        return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnQuit);
    }  //  if (a_durationMs < 0) {
    return EvLoopInvokerLoopWithTimeoutEvLoopThr(a_instance, a_durationMs);
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


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


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


static void EventLoopInvokerInfiniteEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    xcb_generic_event_t* event;
    while ( a_instance->flags.rd.shouldRun_true ) {
        event = xcb_wait_for_event(a_instance->connection);
        if(event){
            EventLoopInvokerHandleSingleEventInline(a_instance,event);
            free(event);
        }  //  if(event){
    }  //  while ( a_instance->flags.rd.shouldRun_true ) {
}


static int EvLoopInvokerLoopWithTimeoutEvLoopThr(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, int64_t a_durationMs) CPPUTILS_NOEXCEPT  
{
    const int xcb_fd = xcb_get_file_descriptor(a_instance->connection);
    const int cnMaxFd = xcb_fd + 1;
    int nSelectRet;
    time_t currentTime, startTime;
    int64_t durationRemaining = a_durationMs;
    struct timeval timeout;
    fd_set fds;
    xcb_generic_event_t* event;

    startTime = time(&startTime);

    do {
        timeout.tv_sec = CPPUTILS_STATIC_CAST(time_t,durationRemaining/1000);
        timeout.tv_usec = CPPUTILS_STATIC_CAST(suseconds_t,(durationRemaining%1000)*1000);
        FD_ZERO(&fds);
        FD_SET(xcb_fd, &fds);
        
        nSelectRet = select(cnMaxFd, &fds, CPPUTILS_NULL, CPPUTILS_NULL, &timeout);
        switch(nSelectRet){
        case -1:
            CinternalSleepInterruptableMs(1000);
            CInternalLogError("Select error");
            break;
        case 0:
            return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnTimeout);
        default:
            break;
        }  //  switch(nSelectRet){
        
        while ((event = xcb_poll_for_event(a_instance->connection)) != CPPUTILS_NULL) {
            EventLoopInvokerHandleSingleEventInline(a_instance,event);
            free(event);
        }  //  while ((event = xcb_poll_for_event(a_instance->connection)) != CPPUTILS_NULL) {

        currentTime = time(&currentTime);
        durationRemaining = a_durationMs - CPPUTILS_STATIC_CAST(int64_t, currentTime-startTime);

    } while ((durationRemaining >= 0)&&(a_instance->flags.rd.shouldRun_true));

    if (a_instance->flags.rd.shouldRun_false) {
        return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnQuit);
    }

    return CPPUTILS_STATIC_CAST(int, EvLoopInvokerLoopReturnTimeout);
}



CPPUTILS_END_C


#endif  //  #if defined(__linux__) || defined(__linux)
