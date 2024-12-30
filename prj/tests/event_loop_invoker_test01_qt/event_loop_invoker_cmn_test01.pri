#
# repo:		event_loop_invoker
# file:		event_loop_invoker_test01.pro
# path:		prj/tests/event_loop_invoker_test01_qt/event_loop_invoker_test01.pro
# created on:	2024 Dec 08
# created by:	Davit Kalantaryan (davit.kalantaryan@desy.de)
#

message("!!! $${PWD}/event_loop_invoker_cmn_test01.pri")
include ( "$${PWD}/../../common/common_qt/flagsandsys_common.pri" )

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
    OBJECTIVE_SOURCES += "$${eventLoopInvokerRepoRoot}/src/core/event_loop_invoker_apple.mm"
} else:linux {
    LIBS += -pthread
    LIBS += -ldl
    LIBS += -lxcb
}

SOURCES	+= "$${eventLoopInvokerRepoRoot}/src/tests/main_event_loop_invoker_test01.c"
SOURCES += $$files($${eventLoopInvokerRepoRoot}/src/core/*.c,true)
SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_logger.c"

HEADERS += $$files($${eventLoopInvokerRepoRoot}/src/core/*.h,true)
HEADERS += $$files($${eventLoopInvokerRepoRoot}/include/*.h,true)

OTHER_FILES += $$files($${eventLoopInvokerRepoRoot}/src/core/*.mm,true)
