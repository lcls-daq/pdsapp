libnames := tools l3test l3sacla l3saclacompound

#CPPFLAGS += -D_FILE_OFFSET_BITS=64 -fopenmp
# 
#libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc RecorderQ.cc DgSummary.cc PnccdShuffle_OMP.cc
#libslib_tools := $(USRLIBDIR)/gomp

CPPFLAGS += -D_FILE_OFFSET_BITS=64
 
libsrcs_tools := EventTest.cc EventOptions.cc Recorder.cc RecorderQ.cc DgSummary.cc PnccdShuffle.cc CspadShuffle.cc StripTransient.cc MonReqServer.cc

libsrcs_l3test := L3TestModule.cc
libincs_l3test := pdsdata/include ndarray/include boost/include 

libsrcs_l3sacla := L3SACLAModule.cc
libincs_l3sacla := pdsdata/include ndarray/include boost/include 

libsrcs_l3saclacompound := L3SACLACompoundModule.cc
libincs_l3saclacompound := pdsdata/include ndarray/include boost/include 

tgtnames := event segtest sourcetest bldtest source montest showPartitions killPartition control bldClientTest bldServerTest bldMonitor xtcdump currentexp showPlatform
#tgtnames := segtest sourcetest bldtest source montest showPartitions killPartition control bldClientTest bldServerTest xtcdump currentexp
#tgtnames := findSource

commonlibs := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
commonlibs += pds/pnccdFrameV0

liblibs_tools := offlinedb/mysqlclient offlinedb/offlinedb pds/offlineclient
libincs_tools := offlinedb/include pdsdata/include ndarray/include boost/include 

tgtsrcs_event := event.cc
tgtlibs_event := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/indexdata pdsdata/smalldata
tgtslib_event := $(USRLIBDIR)/rt
tgtincs_event := offlinedb/include pdsdata/include

tgtsrcs_segtest := segtest.cc
tgtlibs_segtest := $(commonlibs)
tgtslib_segtest := $(USRLIBDIR)/rt
tgtincs_segtest := pdsdata/include ndarray/include boost/include  

tgtsrcs_control := control.cc
tgtlibs_control := $(commonlibs)
tgtslib_control := $(USRLIBDIR)/rt
tgtincs_control := pdsdata/include ndarray/include boost/include  

tgtsrcs_source := source.cc
tgtlibs_source := $(commonlibs)
tgtslib_source := $(USRLIBDIR)/rt
tgtincs_source := pdsdata/include

tgtsrcs_sourcetest := sourcetest.cc
tgtlibs_sourcetest := $(commonlibs)
tgtslib_sourcetest := $(USRLIBDIR)/rt
tgtincs_sourcetest := pdsdata/include

tgtsrcs_bldtest := bldtest.cc
tgtlibs_bldtest := $(commonlibs)
tgtslib_bldtest := $(USRLIBDIR)/rt
tgtincs_bldtest := pdsdata/include

tgtsrcs_montest := montest.cc
tgtlibs_montest := $(commonlibs) pds/mon
tgtslib_montest := $(USRLIBDIR)/rt
tgtincs_montest := pdsdata/include ndarray/include boost/include 

tgtsrcs_showPartitions := showPartitions.cc
tgtlibs_showPartitions := $(commonlibs)
tgtslib_showPartitions := $(USRLIBDIR)/rt
tgtincs_showPartitions := pdsdata/include

tgtsrcs_killPartition := killPartition.cc
tgtlibs_killPartition := $(commonlibs)
tgtslib_killPartition := $(USRLIBDIR)/rt
tgtincs_killPartition := pdsdata/include

tgtsrcs_showPlatform := showPlatform.cc
tgtlibs_showPlatform := $(commonlibs)
tgtslib_showPlatform := $(USRLIBDIR)/rt
tgtincs_showPlatform := pdsdata/include ndarray/include boost/include


tgtsrcs_bldClientTest := bldClientTest.cc bldClientTest.hh
tgtlibs_bldClientTest := pds/service pdsdata/xtcdata
tgtslib_bldClientTest := $(USRLIBDIR)/rt

tgtsrcs_bldServerTest := bldServerTest.cpp bldServerTest.h
tgtlibs_bldServerTest := pds/service
tgtlibs_bldServerTest += $(commonlibs)
tgtslib_bldServerTest := $(USRLIBDIR)/rt
tgtincs_bldServerTest := pdsdata/include

tgtsrcs_bldMonitor := bldMonitor.cc bldMonitor.hh 
tgtlibs_bldMonitor := $(liblibs_tools) $(commonlibs) pdsapp/tools pdsdata/indexdata pdsdata/smalldata
tgtslib_bldMonitor := $(USRLIBDIR)/rt
tgtincs_bldMonitor := pdsdata/include

tgtsrcs_xtcdump := xtcdump.cc
tgtlibs_xtcdump := $(commonlibs)
tgtslib_xtcdump := $(USRLIBDIR)/rt
tgtincs_xtcdump := pdsdata/include

tgtsrcs_currentexp := currentexp.cc
tgtlibs_currentexp := $(commonlibs)
tgtlibs_currentexp += pds/offlineclient
tgtlibs_currentexp += offlinedb/mysqlclient offlinedb/offlinedb
tgtslib_currentexp := $(USRLIBDIR)/rt
tgtincs_currentexp := offlinedb/include offlineclient

libnames += padmon
libsrcs_padmon := PadMonServer.cc CspadShuffle.cc
libincs_padmon := pdsdata/include ndarray/include boost/include   

tgtnames += padmonservertest
tgtsrcs_padmonservertest := padmonservertest.cc
tgtlibs_padmonservertest := pdsapp/padmon pds/configdata pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_padmonservertest := $(USRLIBDIR)/rt
tgtincs_padmonservertest := pdsdata/include ndarray/include boost/include 

tgtnames += epixmonservertest
tgtsrcs_epixmonservertest := epixmonservertest.cc
tgtlibs_epixmonservertest := pdsapp/padmon pds/configdata pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_epixmonservertest := $(USRLIBDIR)/rt

#tgtnames += feboPadmonservertest
#tgtsrcs_feboPadmonservertest := feboPadmonservertest.cc
#tgtlibs_feboPadmonservertest := pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/fexampdata
#tgtslib_feboPadmonservertest := $(USRLIBDIR)/rt


#tgtnames += epicsmonservertest
tgtsrcs_epicsmonservertest := epicsmonservertest.cc EpicsMonServer.cc
tgtlibs_epicsmonservertest := pdsapp/epicsmon pdsdata/xtcdata pdsdata/appdata pdsdata/epics
tgtslib_epicsmonservertest := $(USRLIBDIR)/rt
tgtincs_epicsmonservertest := pdsdata/include ndarray/include boost/include 

tgtsrcs_findSource := findSource.cc
tgtlibs_findSource := $(commonlibs)
tgtslib_findSource := $(USRLIBDIR)/rt
tgtincs_findSource := pdsdata/include

