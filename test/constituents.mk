libnames := ibtest ibtestw

libsrcs_ibtest  := ibcommon.cc

libsrcs_ibtestw := ibcommonw.cc
libslib_ibtestw := ibverbs

tgtnames := timestampReceiver sqlDbTest timerResolution
ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames += princetonCameraTest
endif

ifeq ($(findstring rhel7,$(tgt_arch)),)
tgtnames += andorStandAlone
tgtnames += andorDualStandAlone
endif

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

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

tgtnames := ibhosts
tgtsrcs_ibhosts := ibhosts.cc
tgtslib_ibhosts := ibverbs rt

tgtsrcs_ibrdma := ibrdma.cc
tgtslib_ibrdma := ibverbs

#
#  ibv_reg_mr behaves differently if it is linked from a shared library!
#
tgtsrcs_ibrdmac := ibrdmac.cc ibcommon.cc
#tgtlibs_ibrdmac := pdsapp/ibtest
tgtslib_ibrdmac := ibverbs pthread

tgtnames := iboutlet ibinlet ibrdma ibrdmac
tgtsrcs_iboutlet := iboutlet.cc ibcommon.cc
tgtincs_iboutlet := pdsdata/include ndarray/include boost/include
tgtlibs_iboutlet := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility
#tgtlibs_iboutlet += pdsapp/ibtest
tgtslib_iboutlet := ibverbs rt pthread

tgtsrcs_ibinlet := ibinlet.cc ibcommon.cc
tgtincs_ibinlet := pdsdata/include ndarray/include boost/include
tgtlibs_ibinlet := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility
#tgtlibs_ibinlet += pdsapp/ibtest
tgtslib_ibinlet := ibverbs rt pthread

tgtnames := ibrdmac ibrdma ibinlet iboutlet

tgtsrcs_iboutletw := iboutletw.cc
tgtincs_iboutletw := pdsdata/include ndarray/include boost/include
tgtlibs_iboutletw := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility
tgtlibs_iboutletw += pdsapp/ibtestw
tgtslib_iboutletw := ibverbs rt pthread

tgtsrcs_ibinletw := ibinletw.cc
tgtincs_ibinletw := pdsdata/include ndarray/include boost/include
tgtlibs_ibinletw := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility
tgtlibs_ibinletw += pdsapp/ibtestw
tgtslib_ibinletw := ibverbs rt pthread

tgtsrcs_casttest := casttest.cc
tgtlibs_casttest := pds/service
tgtlibs_casttest += pdsdata/xtcdata
tgtslib_casttest := rt

tgtnames := ibinletw iboutletw casttest
