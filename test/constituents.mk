libnames :=

libsrcs_test :=


tgtnames := timestampReceiver sqlDbTest timerResolution
ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames += princetonCameraTest andorStandAlone andorDualStandAlone archonStandAlone
endif

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames += andorStandAlone andorDualStandAlone archonStandAlone jungfrauStandAlone 
endif

commonlibs	:= pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
commonlibs	+= pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
commonlibs	+= pds/config pds/configdbc pds/confignfs pds/configsql
commonlibs	+= offlinedb/mysqlclient

tgtsrcs_archonStandAlone := archonStandAlone.cc
tgtlibs_archonStandAlone := $(commonlibs) pds/archon
tgtslib_archonStandAlone := dl pthread rt

tgtsrcs_jungfrauStandAlone := jungfrauStandAlone.cc
tgtincs_jungfrauStandAlone := pdsdata/include ndarray/include boost/include
tgtlibs_jungfrauStandAlone := $(commonlibs) pds/jungfrau slsdet/SlsDetector
tgtslib_jungfrauStandAlone := dl pthread rt

tgtsrcs_princetonCameraTest := princetonCameraTest.cc
tgtlibs_princetonCameraTest := pds/princetonutil pvcam/pvcam
#tgtlibs_princetonCameraTest := pvcam/pvcamtest
tgtslib_princetonCameraTest := dl pthread rt

tgtsrcs_andorStandAlone := andorStandAlone.cc
tgtlibs_andorStandAlone := pds/andorutil andor/andor
tgtslib_andorStandAlone := dl pthread rt

tgtsrcs_andorDualStandAlone := andorDualStandAlone.cc
tgtlibs_andorDualStandAlone := pds/andorutil andor/andor
tgtslib_andorDualStandAlone := dl pthread rt

tgtsrcs_timestampReceiver := timestampReceiver.cc

tgtsrcs_sqlDbTest := sqlDbTest.cc
tgtincs_sqlDbTest := pdsdata/include ndarray/include boost/include
tgtlibs_sqlDbTest := pds/config pds/configdbc pds/confignfs pds/configsql offlinedb/mysqlclient
tgtlibs_sqlDbTest += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
tgtlibs_sqlDbTest += pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_sqlDbTest += pdsapp/configdb
tgtslib_sqlDbTest := rt

tgtsrcs_timerResolution := timerResolution.cc
tgtlibs_timerResolution :=
tgtslib_timerResolution := dl pthread rt
