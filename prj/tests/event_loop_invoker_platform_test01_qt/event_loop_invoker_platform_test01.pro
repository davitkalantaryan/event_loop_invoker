#
# repo:		event_loop_invoker
# file:		event_loop_invoker_platform_test01.pro
# path:		prj/tests/event_loop_invoker_test01_qt/event_loop_invoker_platform_test01.pro
# created on:	2024 Dec 08
# created by:	Davit Kalantaryan (davit.kalantaryan@desy.de)
#

message("!!! $${_PRO_FILE_}")
include ( "$${PWD}/../../common/common_qt/flagsandsys_common_private.pri" )

TEMPLATE = subdirs
#CONFIG += ordered

SUBDIRS	+=  "$${PWD}/event_loop_invoker_platform_c_test01.pro"
SUBDIRS	+=  "$${PWD}/event_loop_invoker_platform_cpp_test01.pro"
