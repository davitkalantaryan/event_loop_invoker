//
// repo:            private-input_monitor
// file:			privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// path:			src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp
// created on:		2024 Dec 05
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>


#ifdef _WIN32

#include <event_loop_invoker/event_loop_invoker_platform.h>
#include <cinternal/bistateflags.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C

#pragma comment(lib,"User32.lib")

#define EVENT_LOOP_INVOKER_CLASS_NAME		        "EventLoopInvokerFncWindowClassName"
#define EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK		(WM_USER + 2)
#define EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK		(WM_USER + 3)


struct EvLoopInvokerEventsMonitor {
    struct EvLoopInvokerEventsMonitor* prev, * next;
    EvLoopInvokerTypeEventMonitor   clbk;
    void* clbkData;
};


struct EvLoopInvokerHandle {
    struct EvLoopInvokerEventsMonitor*      pFirstMonitor;
    HANDLE							        waitGuiThreadSema;
    HANDLE							        guiThread;
    HINSTANCE						        hInstance;
    HWND							        functionalWnd;
    DWORD							        dwGuiThreadId;
    ATOM                                    regClassReturn;
    ATOM                                    reserved01;
    void*                                   reserved02;
    CPPUTILS_BISTATE_FLAGS_UN(
        shouldRun,
        hasError
    )flags;
};

static DWORD WINAPI EventLoopInvokerCallbacksThread(LPVOID a_lpThreadParameter) CPPUTILS_NOEXCEPT;
static VOID NTAPI EvLoopInvokerUserApcClbk(_In_ ULONG_PTR a_arg) CPPUTILS_NOEXCEPT {(void)a_arg;}


static inline bool EvLoopInvokerCallAllMonitorsInline(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, MSG* CPPUTILS_ARG_NN a_event) {
    struct EvLoopInvokerEventsMonitor* pMonitorNext, * pMonitor = a_instance->pFirstMonitor;
    while (pMonitor) {
        pMonitorNext = pMonitor->next;
        if ((*(pMonitor->clbk))(a_instance,pMonitor->clbkData, a_event)) {
            return true;
        }
        pMonitor = pMonitorNext;
    }  //  while(pMonitor){
    return false;
}


EVLOOPINVK_EXPORT struct EvLoopInvokerHandle* EvLoopInvokerCreateHandleEx(const void* a_inp) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)calloc(1, sizeof(struct EvLoopInvokerHandle));
    if (!pRetStr) {
        return pRetStr;
    }

    CPPUTILS_STATIC_CAST(void, a_inp);

    pRetStr->flags.wr_all = CPPUTILS_BISTATE_MAKE_ALL_BITS_FALSE;
    pRetStr->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_TRUE;

    pRetStr->waitGuiThreadSema = CreateSemaphoreA(CPPUTILS_NULL, 0, 100, CPPUTILS_NULL);
    if (!(pRetStr->waitGuiThreadSema)) {
        free(pRetStr);
        CInternalLogError("Unable to create semaphore thread");
        return CPPUTILS_NULL;
    }

    pRetStr->guiThread = CreateThread(CPPUTILS_NULL, 0, &EventLoopInvokerCallbacksThread, pRetStr, 0, &(pRetStr->dwGuiThreadId));
    if (!(pRetStr->guiThread)) {
        CloseHandle(pRetStr->waitGuiThreadSema);
        free(pRetStr);
        CInternalLogError("Unable to create callback thread");
        return CPPUTILS_NULL;
    }

    WaitForSingleObject(pRetStr->waitGuiThreadSema, INFINITE);
    CloseHandle(pRetStr->waitGuiThreadSema);
    pRetStr->waitGuiThreadSema = CPPUTILS_NULL;
    if (pRetStr->flags.rd.hasError_true) {
        WaitForSingleObject(pRetStr->guiThread, INFINITE);
        CloseHandle(pRetStr->guiThread);
        free(pRetStr);
        CInternalLogError("Error in the GUI thread");
        return CPPUTILS_NULL;
    }

    return pRetStr;
}


EVLOOPINVK_EXPORT void EvLoopInvokerCleanHandle(struct EvLoopInvokerHandle* a_instance) CPPUTILS_NOEXCEPT
{
    if (a_instance) {
        a_instance->flags.wr.shouldRun = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
        PostThreadMessageA(a_instance->dwGuiThreadId, WM_USER + 1, 0, 0);
        QueueUserAPC(&EvLoopInvokerUserApcClbk, a_instance->guiThread, 0);
        WaitForSingleObject(a_instance->guiThread, INFINITE);
        CloseHandle(a_instance->guiThread);
        free(a_instance);
    }
}


EVLOOPINVK_EXPORT void* EvLoopInvokerCallFuncionBlocked(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerBlockedClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    void* const pRet = (void*)SendMessageA(a_instance->functionalWnd, EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK, (WPARAM)a_fnc, (LPARAM)a_pData);
    return pRet;
}


EVLOOPINVK_EXPORT void  EvLoopInvokerCallFuncionAsync(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerAsyncClbk a_fnc, void* a_pData) CPPUTILS_NOEXCEPT
{
    PostMessageA(a_instance->functionalWnd, EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK, (WPARAM)a_fnc, (LPARAM)a_pData);
}


/*/// platform specific api  ///*/

EVLOOPINVK_EXPORT struct EvLoopInvokerEventsMonitor* EvLoopInvokerRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, EvLoopInvokerTypeEventMonitor a_fnc, void* a_clbkData)
{
    struct EvLoopInvokerEventsMonitor* const pMonitor = (struct EvLoopInvokerEventsMonitor*)calloc(1, sizeof(struct EvLoopInvokerEventsMonitor));
    if (!pMonitor) {
        return CPPUTILS_NULL;
    }

    pMonitor->clbk = a_fnc;
    pMonitor->clbkData = a_clbkData;
    pMonitor->prev = CPPUTILS_NULL;
    pMonitor->next = a_instance->pFirstMonitor;
    if (a_instance->pFirstMonitor) {
        a_instance->pFirstMonitor->prev = pMonitor;
    }
    a_instance->pFirstMonitor = pMonitor;
    return pMonitor;
}


EVLOOPINVK_EXPORT void EvLoopInvokerUnRegisterEventsMonitor(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance, struct EvLoopInvokerEventsMonitor* a_eventsMonitor)
{
    if (a_eventsMonitor) {
        if (a_eventsMonitor->next) {
            a_eventsMonitor->next->prev = a_eventsMonitor->prev;
        }
        if (a_eventsMonitor->prev) {
            a_eventsMonitor->prev->next = a_eventsMonitor->next;
        }
        else {
            a_instance->pFirstMonitor = a_eventsMonitor->next;
        }
        free(a_eventsMonitor);
    }  //  if(a_eventsMonitor){
}


/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static void EventLoopInvokerClearInstanceFromEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static int EventLoopInvokerConfigureInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT;
static LRESULT CALLBACK EventLoopInvokerWndPproc(HWND a_hWnd, UINT a_msgNumber, WPARAM a_wParam, LPARAM a_lParam) CPPUTILS_NOEXCEPT;


static DWORD WINAPI EventLoopInvokerCallbacksThread(LPVOID a_lpThreadParameter) CPPUTILS_NOEXCEPT
{
    MSG msg;
    struct EvLoopInvokerHandle* const pRetStr = (struct EvLoopInvokerHandle*)a_lpThreadParameter;
    const int nRet = EventLoopInvokerConfigureInstanceInEventLoop(pRetStr);

    if (nRet) {
        EventLoopInvokerClearInstanceFromEventLoop(pRetStr);
        ReleaseSemaphore(pRetStr->waitGuiThreadSema, 1, CPPUTILS_NULL);
        ExitThread(1);
    }

    ReleaseSemaphore(pRetStr->waitGuiThreadSema, 1, CPPUTILS_NULL);

    while (pRetStr->flags.rd.shouldRun_true && GetMessageA(&msg, CPPUTILS_NULL, 0, 0)) {
        TranslateMessage(&msg);
        if (!EvLoopInvokerCallAllMonitorsInline(pRetStr, &msg)) {
            DispatchMessageA(&msg);
        }
    }

    EventLoopInvokerClearInstanceFromEventLoop(pRetStr);

    ExitThread(0);
}


static void EventLoopInvokerClearInstanceFromEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    if (a_instance->functionalWnd) {
        DestroyWindow(a_instance->functionalWnd);
        a_instance->functionalWnd = CPPUTILS_NULL;
    }

    if (a_instance->regClassReturn) {
        UnregisterClassA(EVENT_LOOP_INVOKER_CLASS_NAME, a_instance->hInstance);
        a_instance->regClassReturn = 0;
    }
}


static int EventLoopInvokerConfigureInstanceInEventLoop(struct EvLoopInvokerHandle* CPPUTILS_ARG_NN a_instance) CPPUTILS_NOEXCEPT
{
    WNDCLASSA regClassData;

    a_instance->flags.wr.hasError = CPPUTILS_BISTATE_MAKE_BITS_TRUE;

    a_instance->hInstance = GetModuleHandleA(CPPUTILS_NULL);
    if (!(a_instance->hInstance)) {
        CInternalLogError("Getting application instance failed");
        return 1;
    }

    regClassData.style = 0;
    regClassData.lpfnWndProc = &EventLoopInvokerWndPproc;
    regClassData.cbClsExtra = 0;
    regClassData.cbWndExtra = (int)sizeof(struct EvLoopInvokerHandle*);
    regClassData.hInstance = a_instance->hInstance;
    regClassData.hIcon = CPPUTILS_NULL;
    regClassData.hCursor = CPPUTILS_NULL;
    regClassData.hbrBackground = CPPUTILS_NULL;
    regClassData.lpszMenuName = CPPUTILS_NULL;
    regClassData.lpszClassName = EVENT_LOOP_INVOKER_CLASS_NAME;
    a_instance->regClassReturn = RegisterClassA(&regClassData);
    if (!(a_instance->regClassReturn)) {
        CInternalLogError("Registering functional class failed");
        return 1;
    }

    a_instance->functionalWnd = CreateWindowA(
        EVENT_LOOP_INVOKER_CLASS_NAME,	/* Classname */
        "",								/* Title Text */
        0,								/* Styles */
        0,								/* X initial */
        0,								/* Y initial */
        0,								/* Width initial */
        0,								/* Heigh initial */
        HWND_MESSAGE,					/* This is a  message-only window */
        //NULL,							/* This is a  message-only window */
        CPPUTILS_NULL,					/* Menu */ /* No menu */
        a_instance->hInstance,			/* Program Instance handler */
        CPPUTILS_NULL					/* No Window Creation data */
    );
    if (!a_instance->functionalWnd) {
        CInternalLogError("Creation of functional window failed");
        return 1;
    }

    SetLastError(0);
    if (!SetWindowLongPtrA(a_instance->functionalWnd, 0, (LONG_PTR)a_instance)) {
        const DWORD dwLastError = GetLastError();
        if (dwLastError != ERROR_SUCCESS) {
            CInternalLogError("Adding window specific data failed");
            return 1;
        }
    }

    a_instance->flags.wr.hasError = CPPUTILS_BISTATE_MAKE_BITS_FALSE;
    return 0;
}


static LRESULT CALLBACK EventLoopInvokerWndPproc(HWND a_hWnd, UINT a_msgNumber, WPARAM a_wParam, LPARAM a_lParam) CPPUTILS_NOEXCEPT
{
    struct EvLoopInvokerHandle* const pInstance = (struct EvLoopInvokerHandle*)GetWindowLongPtrA(a_hWnd, 0);

    switch (a_msgNumber) {
    case EVENT_LOOP_INVOKER_BLOCKED_CALLFNC_HOOK: {
        const EvLoopInvokerBlockedClbk blockedClbk = (EvLoopInvokerBlockedClbk)a_wParam;
        void* const pArg = (void*)a_lParam;
        void* const pRet = (*blockedClbk)(pInstance,pArg);
        return (LRESULT)pRet;
    }break;
    case EVENT_LOOP_INVOKER_ASYNC_CALLFNC_HOOK: {
        const EvLoopInvokerAsyncClbk asyncClbk = (EvLoopInvokerAsyncClbk)a_wParam;
        void* const pArg = (void*)a_lParam;
        (*asyncClbk)(pInstance,pArg);
        return (LRESULT)0;
    }break;
    default:
        break;
    }  //  switch (a_msgNumber){

    return DefWindowProcA(a_hWnd, a_msgNumber, a_wParam, a_lParam);
}


CPPUTILS_END_C


#endif  //  #ifdef _WIN32
