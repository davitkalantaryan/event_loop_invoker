#
# file:			any_quick_test.pro
# path:			prj/tests/any_quick_test_qt/any_quick_test.pro
# created on:	2021 Mar 07
# created by:	Davit Kalantaryan
#

message("!!! $${_PRO_FILE_}")
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
    OBJECTIVE_SOURCES += "$${privInputMonitorRepoRoot}/src/core/base/privinpmonitor_core_base_input_monitor_mac.mm"
} else {
    LIBS += -pthread
    LIBS += -ldl
}

SOURCES	+= "$${privInputMonitorRepoRoot}/src/tests/main_input_monitor_guithr_test.cpp"
SOURCES	+= "$${privInputMonitorRepoRoot}/src/core/base/privinpmonitor_core_base_input_monitor_common.cpp"
SOURCES	+= "$${privInputMonitorRepoRoot}/src/core/base/privinpmonitor_core_base_input_monitor_guithr_windows.cpp"
SOURCES	+= "$${privInputMonitorRepoRoot}/src/core/base/privinpmonitor_core_base_input_monitor_linux.cpp"

HEADERS += $$files($${privInputMonitorRepoRoot}/include/*.h,true)
HEADERS += $$files($${privInputMonitorRepoRoot}/src/core/*.h,true)
