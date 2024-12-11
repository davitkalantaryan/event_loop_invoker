#
# repo:		event_loop_invoker
# file:		event_loop_invoker_test01.pro
# path:		prj/tests/event_loop_invoker_test01_qt/event_loop_invoker_test01.pro
# created on:	2024 Dec 08
# created by:	Davit Kalantaryan (davit.kalantaryan@desy.de)
#

message("!!! $${_PRO_FILE_}")
include ( "$${PWD}/../../common/common_qt/flagsandsys_common_private.pri" )

DESTDIR     = "$${ArifactFinal}/test"

isEmpty(MAKE_DEBUG_FROM_EVENTS) {
    MAKE_DEBUG_FROM_EVENTS = $$(MAKE_DEBUG_FROM_EVENTS)
    isEmpty(MAKE_DEBUG_FROM_EVENTS) {
	message("-- MAKE_DEBUG_FROM_EVENTS is not defined")
    } else {
	DEFINES += MAKE_DEBUG_FROM_EVENTS
	message("++ MAKE_DEBUG_FROM_EVENTS is defined")
    }
}

QT += gui
QT += core
QT -= widgets
CONFIG += console

win32{
    QMAKE_CXXFLAGS -= /Wall /WX
} else:mac {
    OBJECTIVE_SOURCES += "$${eventLoopInvokerRepoRoot}/src/core/event_loop_invoker_mac.mm"
    OBJECTIVE_SOURCES += "$${eventLoopInvokerRepoRoot}/src/tests/event_loop_invoker_platform_test01_mac.mm"
} else:linux {
    LIBS += -pthread
    LIBS += -ldl
    LIBS += -lxcb
}

SOURCES	+= "$${eventLoopInvokerRepoRoot}/src/tests/main_event_loop_invoker_platform_test01.c"
SOURCES += "$${eventLoopInvokerRepoRoot}/src/core/event_loop_invoker_windows.c"
SOURCES += "$${eventLoopInvokerRepoRoot}/src/core/event_loop_invoker_linux.c"
SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_logger.c"

HEADERS += $$files($${eventLoopInvokerRepoRoot}/include/*.h,true)

OTHER_FILES += "$${eventLoopInvokerRepoRoot}/src/core/event_loop_invoker_mac.mm"
