#
# repo:		event_loop_invoker
# name:		flagsandsys_common_private.pri
# path:		prj/common/common_qt/flagsandsys_common_private.pri
# created on:   2024 Dec 08
# created by:   Davit Kalantaryan (davit.kalantaryan@desy.de)
# usage:	Use this qt include file to calculate some platform specific stuff
#


message("!!! $${PWD}/flagsandsys_common_private.pri")

isEmpty(eventLoopInvokerFlagsAndSysCommonPrivateIncluded){
    eventLoopInvokerFlagsAndSysCommonPrivateIncluded = 1

    include("$${PWD}/flagsandsys_common.pri")
    
    exists($${eventLoopInvokerRepoRoot}/src/include) {
	INCLUDEPATH += $${eventLoopInvokerRepoRoot}/src/include
    } else {
	message("$${eventLoopInvokerRepoRoot}/src/include directory does not exist")
    }
}
