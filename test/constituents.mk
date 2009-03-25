libnames := test
 
libsrcs_test := EventTest.cc EventOptions.cc Recorder.cc

tgtnames := recordertest eventtest segtest controltest sourcetest bldtest source montest showPartitions killPartition
tgtnames += xtcreader

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsapp/test

tgtsrcs_recordertest := recordertest.cc
tgtlibs_recordertest := $(commonlibs)
tgtslib_recordertest := /usr/lib/rt

tgtsrcs_eventtest := eventtest.cc
tgtlibs_eventtest := $(commonlibs)
tgtslib_eventtest := /usr/lib/rt

tgtsrcs_segtest := segtest.cc
tgtlibs_segtest := $(commonlibs)
tgtslib_segtest := /usr/lib/rt

tgtsrcs_controltest := controltest.cc
tgtlibs_controltest := $(commonlibs)
tgtslib_controltest := /usr/lib/rt

tgtsrcs_source := source.cc
tgtlibs_source := $(commonlibs)
tgtslib_source := /usr/lib/rt

tgtsrcs_sourcetest := sourcetest.cc
tgtlibs_sourcetest := $(commonlibs)
tgtslib_sourcetest := /usr/lib/rt

tgtsrcs_bldtest := bldtest.cc
tgtlibs_bldtest := $(commonlibs)
tgtslib_bldtest := /usr/lib/rt

tgtsrcs_montest := montest.cc
tgtlibs_montest := $(commonlibs) pds/mon
tgtslib_montest := /usr/lib/rt

tgtsrcs_showPartitions := showPartitions.cc
tgtlibs_showPartitions := $(commonlibs)
tgtslib_showPartitions := /usr/lib/rt

tgtsrcs_killPartition := killPartition.cc
tgtlibs_killPartition := $(commonlibs)
tgtslib_killPartition := /usr/lib/rt

#it's very important that xtcreader only depend on pdsdata - cpo
tgtsrcs_xtcreader := xtcreader.cc
tgtlibs_xtcreader := pdsdata/xtcdata pdsdata/acqdata
tgtslib_xtcreader := /usr/lib/rt
