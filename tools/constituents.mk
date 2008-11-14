libnames := test
 
libsrcs_test := EventTest.cc EventOptions.cc Recorder.cc

tgtnames          := recordertest eventtest segtest controltest sourcetest bldtest source

tgtsrcs_recordertest := recordertest.cc
tgtlibs_recordertest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_recordertest := /usr/lib/rt

tgtsrcs_eventtest := eventtest.cc
tgtlibs_eventtest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_eventtest := /usr/lib/rt

tgtsrcs_segtest := segtest.cc
tgtlibs_segtest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_segtest := /usr/lib/rt

tgtsrcs_controltest := controltest.cc
tgtlibs_controltest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_controltest := /usr/lib/rt

tgtsrcs_source := source.cc
tgtlibs_source := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_source := /usr/lib/rt

tgtsrcs_sourcetest := sourcetest.cc
tgtlibs_sourcetest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test
tgtslib_sourcetest := /usr/lib/rt

tgtsrcs_bldtest := bldtest.cc
tgtlibs_bldtest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client 
tgtslib_bldtest := /usr/lib/rt
