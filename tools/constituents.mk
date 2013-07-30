libnames := tools

#CPPFLAGS += -D_FILE_OFFSET_BITS=64 -fopenmp
# 
#libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc RecorderQ.cc DgSummary.cc PnccdShuffle_OMP.cc
#libslib_tools := $(USRLIBDIR)/gomp

CPPFLAGS += -D_FILE_OFFSET_BITS=64
 
libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc RecorderQ.cc DgSummary.cc PnccdShuffle.cc CspadShuffle.cc StripTransient.cc

tgtnames := event segtest sourcetest bldtest source montest showPartitions killPartition control bldClientTest bldServerTest observertest bldMonitor eventp xtcdump currentexp

commonlibs := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
commonlibs += pds/pnccdFrameV0
liblibs_tools := pdsdata/cspaddata pdsdata/pnccddata pdsdata/fexampdata
liblibs_tools += offlinedb/mysqlclient offlinedb/offlinedb pds/offlineclient
libincs_tools := offlinedb/include

tgtsrcs_event := event.cc
tgtlibs_event := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/indexdata pdsdata/evrdata
tgtslib_event := $(USRLIBDIR)/rt
tgtincs_event := offlinedb/include

tgtsrcs_eventp := eventp.cc ParasiticRecorder.cc
tgtlibs_eventp := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/indexdata pdsdata/evrdata pds/offlineclient
tgtlibs_eventp += offlinedb/mysqlclient offlinedb/offlinedb
tgtslib_eventp := $(USRLIBDIR)/rt
tgtincs_eventp := offlinedb/include

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
tgtlibs_bldServerTest += $(commonlibs)
tgtslib_bldServerTest := $(USRLIBDIR)/rt

tgtsrcs_bldMonitor := bldMonitor.cc bldMonitor.hh 
tgtlibs_bldMonitor := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/evrdata pdsdata/indexdata 
tgtslib_bldMonitor := $(USRLIBDIR)/rt

tgtsrcs_observertest := observertest.cc
tgtlibs_observertest := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/evrdata pdsdata/indexdata 
tgtslib_observertest := $(USRLIBDIR)/rt

tgtsrcs_xtcdump := xtcdump.cc
tgtlibs_xtcdump := $(commonlibs)
tgtslib_xtcdump := $(USRLIBDIR)/rt

tgtsrcs_currentexp := currentexp.cc
tgtlibs_currentexp := $(commonlibs)
tgtlibs_currentexp += pds/offlineclient
tgtlibs_currentexp += offlinedb/mysqlclient offlinedb/offlinedb
tgtslib_currentexp := $(USRLIBDIR)/rt
tgtincs_currentexp := offlinedb/include offlineclient

libnames += padmon
libsrcs_padmon := PadMonServer.cc CspadShuffle.cc

tgtnames += padmonservertest
tgtsrcs_padmonservertest := padmonservertest.cc
tgtlibs_padmonservertest := pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/impdata
tgtslib_padmonservertest := $(USRLIBDIR)/rt

tgtnames += epixmonservertest
tgtsrcs_epixmonservertest := epixmonservertest.cc
tgtlibs_epixmonservertest := pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/impdata
tgtslib_epixmonservertest := $(USRLIBDIR)/rt

#tgtnames += feboPadmonservertest
#tgtsrcs_feboPadmonservertest := feboPadmonservertest.cc
#tgtlibs_feboPadmonservertest := pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/fexampdata
#tgtslib_feboPadmonservertest := $(USRLIBDIR)/rt


libnames += epicsmon
libsrcs_epicsmon := EpicsMonServer.cc

tgtnames += epicsmonservertest
tgtsrcs_epicsmonservertest := epicsmonservertest.cc
tgtlibs_epicsmonservertest := pdsapp/epicsmon pdsdata/xtcdata pdsdata/appdata pdsdata/epics
tgtslib_epicsmonservertest := $(USRLIBDIR)/rt
