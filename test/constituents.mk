libnames := test

libsrcs_test := 

tgtnames := princetonCameraTest

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

tgtsrcs_princetonCameraTest := princetonCameraTest.cc
tgtlibs_princetonCameraTest := pvcam/pvcam
tgtslib_princetonCameraTest := dl raw1394 pthread rt
