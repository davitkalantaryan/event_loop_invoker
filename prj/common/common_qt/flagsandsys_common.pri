#
# repo:			cpputils
# name:			flagsandsys_common.pri
# path:			prj/common/common_qt/flagsandsys_common.pri
# created on:   2023 Jun 21
# created by:   Davit Kalantaryan (davit.kalantaryan@desy.de)
# usage:		Use this qt include file to calculate some platform specific stuff
#


message("!!! $${PWD}/flagsandsys_common.pri")

isEmpty(privInputMonitorFlagsAndSysCommonIncluded){
    privInputMonitorFlagsAndSysCommonIncluded = 1

    privInputMonitorRepoRoot = $${PWD}/../../..

    isEmpty(artifactRoot) {
        artifactRoot = $$(artifactRoot)
        isEmpty(artifactRoot) {
            artifactRoot = $${privInputMonitorRepoRoot}
        }
    }

    include("$${privInputMonitorRepoRoot}/contrib/cinternal/prj/common/common_qt/flagsandsys_common.pri")

    INCLUDEPATH += $${privInputMonitorRepoRoot}/include

    LIBS	+= -L$${privInputMonitorRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/lib
    LIBS	+= -L$${privInputMonitorRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/tlib
}
