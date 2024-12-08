#
# repo:		event_loop_invoker
# name:		flagsandsys_common.pri
# path:		prj/common/common_qt/flagsandsys_common.pri
# created on:   2024 Dec 08
# created by:   Davit Kalantaryan (davit.kalantaryan@desy.de)
# usage:	Use this qt include file to calculate some platform specific stuff
#


message("!!! $${PWD}/flagsandsys_common.pri")

isEmpty(eventLoopInvokerFlagsAndSysCommonIncluded){
    eventLoopInvokerFlagsAndSysCommonIncluded = 1

    eventLoopInvokerRepoRoot = $${PWD}/../../..

    isEmpty(artifactRoot) {
        artifactRoot = $$(artifactRoot)
        isEmpty(artifactRoot) {
            artifactRoot = $${eventLoopInvokerRepoRoot}
        }
    }

    include("$${eventLoopInvokerRepoRoot}/contrib/cinternal/prj/common/common_qt/flagsandsys_common.pri")

    INCLUDEPATH += $${eventLoopInvokerRepoRoot}/include
    
    exists($${eventLoopInvokerRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/lib) {
	LIBS += -L$${eventLoopInvokerRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/lib
    }
    exists($${eventLoopInvokerRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/tlib) {
	LIBS += -L$${eventLoopInvokerRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/tlib
    }
}
