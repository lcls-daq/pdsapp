libnames := tools

CPPFLAGS += -D_FILE_OFFSET_BITS=64
 
libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc DgSummary.cc PnccdShuffle.cc

tgtnames := event segtest sourcetest bldtest source montest showPartitions killPartition control bldClientTest bldServerTest observertest bldMonitor

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

tgtsrcs_event := event.cc
tgtlibs_event := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_event := /usr/lib/rt

tgtsrcs_segtest := segtest.cc
tgtlibs_segtest := $(commonlibs)
tgtslib_segtest := /usr/lib/rt

tgtsrcs_control := control.cc
tgtlibs_control := $(commonlibs)
tgtslib_control := /usr/lib/rt

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

tgtsrcs_bldClientTest := bldClientTest.cc bldClientTest.hh
tgtlibs_bldClientTest := pds/service
tgtslib_bldClientTest := /usr/lib/rt

tgtsrcs_bldServerTest := bldServerTest.cpp bldServerTest.h
tgtlibs_bldServerTest := pds/service
tgtslib_bldServerTest := /usr/lib/rt

tgtsrcs_bldMonitor := bldMonitor.cc bldMonitor.hh
tgtlibs_bldMonitor := $(commonlibs)
tgtslib_bldMonitor := /usr/lib/rt

tgtsrcs_observertest := observertest.cc
tgtlibs_observertest := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_observertest := /usr/lib/rt
