libnames := tools

CPPFLAGS += -D_FILE_OFFSET_BITS=64
 
libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc RecorderQ.cc DgSummary.cc PnccdShuffle.cc

tgtnames := event segtest sourcetest bldtest source montest showPartitions killPartition control bldClientTest bldServerTest observertest bldMonitor eventp

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client

tgtsrcs_event := event.cc
tgtlibs_event := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_event := $(USRLIBDIR)/rt

tgtsrcs_eventp := eventp.cc ParasiticRecorder.cc
tgtlibs_eventp := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_eventp := $(USRLIBDIR)/rt

tgtsrcs_segtest := segtest.cc
tgtlibs_segtest := $(commonlibs)
tgtslib_segtest := $(USRLIBDIR)/rt

tgtsrcs_control := control.cc
tgtlibs_control := $(commonlibs)
tgtslib_control := $(USRLIBDIR)/rt

tgtsrcs_source := source.cc
tgtlibs_source := $(commonlibs)
tgtslib_source := $(USRLIBDIR)/rt

tgtsrcs_sourcetest := sourcetest.cc
tgtlibs_sourcetest := $(commonlibs)
tgtslib_sourcetest := $(USRLIBDIR)/rt

tgtsrcs_bldtest := bldtest.cc
tgtlibs_bldtest := $(commonlibs)
tgtslib_bldtest := $(USRLIBDIR)/rt

tgtsrcs_montest := montest.cc
tgtlibs_montest := $(commonlibs) pds/mon
tgtslib_montest := $(USRLIBDIR)/rt

tgtsrcs_showPartitions := showPartitions.cc
tgtlibs_showPartitions := $(commonlibs)
tgtslib_showPartitions := $(USRLIBDIR)/rt

tgtsrcs_killPartition := killPartition.cc
tgtlibs_killPartition := $(commonlibs)
tgtslib_killPartition := $(USRLIBDIR)/rt

tgtsrcs_bldClientTest := bldClientTest.cc bldClientTest.hh
tgtlibs_bldClientTest := pds/service
tgtslib_bldClientTest := $(USRLIBDIR)/rt

tgtsrcs_bldServerTest := bldServerTest.cpp bldServerTest.h
tgtlibs_bldServerTest := pds/service
tgtslib_bldServerTest := $(USRLIBDIR)/rt

tgtsrcs_bldMonitor := bldMonitor.cc bldMonitor.hh 
tgtlibs_bldMonitor := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_bldMonitor := $(USRLIBDIR)/rt

tgtsrcs_observertest := observertest.cc
tgtlibs_observertest := pdsdata/pnccddata pdsdata/opal1kdata $(commonlibs) pdsapp/tools
tgtslib_observertest := $(USRLIBDIR)/rt
