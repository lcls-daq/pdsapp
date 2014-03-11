libnames :=

libsrcs_test :=


tgtnames := andorStandAlone timestampReceiver sqlDbTest
ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames += princetonCameraTest
endif

commonlibs := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

tgtsrcs_princetonCameraTest := princetonCameraTest.cc
tgtlibs_princetonCameraTest := pds/princetonutil pvcam/pvcam
#tgtlibs_princetonCameraTest := pvcam/pvcamtest
tgtslib_princetonCameraTest := dl pthread rt

tgtsrcs_andorStandAlone := andorStandAlone.cc
tgtlibs_andorStandAlone := pds/andorutil andor/andor
tgtslib_andorStandAlone := dl pthread rt

tgtsrcs_timestampReceiver := timestampReceiver.cc

tgtsrcs_sqlDbTest := sqlDbTest.cc
tgtincs_sqlDbTest := pdsdata/include ndarray/include boost/include
tgtlibs_sqlDbTest := pds/config pds/configdbc pds/confignfs pds/configsql offlinedb/mysqlclient
tgtlibs_sqlDbTest += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
tgtlibs_sqlDbTest += pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_sqlDbTest += pdsapp/configdb
tgtslib_sqlDbTest := rt
