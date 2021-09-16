libnames :=

libsrcs_test :=


tgtnames := timestampReceiver sqlDbTest timerResolution
ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames += princetonCameraTest andorStandAlone andorDualStandAlone
endif

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames += andorStandAlone andorDualStandAlone archonStandAlone uxiStandAlone
endif

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
tgtnames += zylaStandAlone andorStandAlone andorDualStandAlone jungfrauStandAlone archonStandAlone picamStandAlone uxiStandAlone
tgtnames += vimbaStandAlone
endif

commonlibs	:= pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
commonlibs	+= pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
commonlibs	+= pds/config pds/configdbc pds/confignfs pds/configsql
commonlibs	+= mysql/mysqlclient

tgtsrcs_archonStandAlone := archonStandAlone.cc
tgtlibs_archonStandAlone := $(commonlibs) pds/archon
tgtslib_archonStandAlone := dl pthread rt

tgtsrcs_uxiStandAlone := uxiStandAlone.cc
tgtlibs_uxiStandAlone := $(commonlibs) pds/configdata
tgtlibs_uxiStandAlone += pds/uxi zeromq/zmq
tgtslib_uxiStandAlone := dl pthread rt

tgtsrcs_jungfrauStandAlone := jungfrauStandAlone.cc
tgtincs_jungfrauStandAlone := pdsdata/include ndarray/include boost/include
tgtlibs_jungfrauStandAlone := $(commonlibs) pds/configdata
tgtlibs_jungfrauStandAlone += pds/jungfrau slsdet/SlsDetector zeromq/zmq
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

tgtsrcs_zylaStandAlone := zylaStandAlone.cc
tgtincs_zylaStandAlone := pdsdata/include ndarray/include boost/include
tgtlibs_zylaStandAlone := $(commonlibs) pds/zyla andor3/atcore andor3/atutility andor3/atcl_bitflow andor3/BFSOciLib.9.05
tgtslib_zylaStandAlone := dl pthread rt

tgtsrcs_vimbaStandAlone := vimbaStandAlone.cc
tgtincs_vimbaStandAlone := pdsdata/include ndarray/include boost/include
tgtlibs_vimbaStandAlone := $(commonlibs) pds/configdata pds/vimba vimba/VimbaC vimba/VimbaImageTransform
tgtslib_vimbaStandAlone := dl pthread rt

libPicam := picam/picam picam/GenApi_gcc40_v2_4 picam/GCBase_gcc40_v2_4 picam/MathParser_gcc40_v2_4 picam/log4cpp_gcc40_v2_4 picam/Log_gcc40_v2_4
libPicam += picam/piac picam/pidi picam/picc picam/pida picam/PvBase picam/PvDevice picam/PvBuffer picam/PvPersistence picam/ftd2xx
libPicam += picam/PvStream picam/PvGenICam picam/PvSerial picam/PtUtilsLib picam/EbUtilsLib
libPicam += picam/PtConvertersLib picam/EbTransportLayerLib

incPicam := picam/include

tgtsrcs_picamStandAlone := picamStandAlone.cc
tgtlibs_picamStandAlone := $(commonlibs)
tgtlibs_picamStandAlone += $(libPicam) pds/picamutils
tgtslib_picamStandAlone := dl pthread rt
tgtincs_picamStandAlone += $(incPicam)


tgtsrcs_timestampReceiver := timestampReceiver.cc

tgtsrcs_sqlDbTest := sqlDbTest.cc
tgtincs_sqlDbTest := pdsdata/include ndarray/include boost/include
tgtlibs_sqlDbTest := pds/config pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_sqlDbTest += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
tgtlibs_sqlDbTest += pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_sqlDbTest += pdsapp/configdb
tgtslib_sqlDbTest := rt

tgtsrcs_timerResolution := timerResolution.cc
tgtlibs_timerResolution :=
tgtslib_timerResolution := dl pthread rt
