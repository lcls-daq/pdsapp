libnames := 

libsrcs_test := 

#tgtnames :=
tgtnames := princetonCameraTest

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

tgtsrcs_princetonCameraTest := princetonCameraTest.cc
tgtlibs_princetonCameraTest := pvcam/pvcam
#tgtlibs_princetonCameraTest := pvcam/pvcamtest
tgtslib_princetonCameraTest := dl pthread rt
