tgtnames := monobs monshm monshmserver offlineobs sxrmon xppmon cximon xcsmon cspadmon
#tgtnames += alive_mon
ifneq ($(findstring x86_64,$(tgt_arch)),)
else
endif

libnames := 

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/configdata pdsdata/xtcdata pdsdata/psddl_pdsdata
#commonlibs += pds/pnccdFrameV0
liblibs_tools := pdsdata/psddl_pdsdata pds/monreq

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := $(USRLIBDIR)/rt
tgtincs_monobs := pdsdata/include ndarray/include boost/include  

tgtsrcs_monshm := monshm.cc CamDisplay.cc AcqDisplay.cc XtcMonitorClient.cc
tgtlibs_monshm += $(commonlibs) pds/mon
tgtslib_monshm := $(USRLIBDIR)/rt
tgtincs_monshm := pdsdata/include ndarray/include boost/include   

tgtsrcs_monshmserver := monshmserver.cc MonComm.cc
tgtlibs_monshmserver := pdsdata/appdata pds/offlineclient offlinedb/mysqlclient offlinedb/offlinedb
tgtlibs_monshmserver += $(commonlibs) pds/mon $(liblibs_tools) pdsapp/tools pdsdata/indexdata pdsdata/smalldata pdsdata/psddl_pdsdata
tgtslib_monshmserver := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_monshmserver := pdsdata/include

tgtsrcs_monreqserver := monreqserver.cc
tgtlibs_monreqserver := pdsdata/appdata pds/offlineclient offlinedb/mysqlclient offlinedb/offlinedb
tgtlibs_monreqserver += $(commonlibs) pds/mon $(liblibs_tools) pdsapp/tools pdsdata/indexdata pdsdata/smalldata pdsdata/psddl_pdsdata
tgtslib_monreqserver := $(USRLIBDIR)/rt
tgtincs_monreqserver := pdsdata/include

tgtsrcs_offlineobs := offlineobs.cc OfflineAppliance.cc
tgtlibs_offlineobs := $(commonlibs) pds/mon pds/offlineclient
tgtlibs_offlineobs += offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com
tgtslib_offlineobs := $(USRLIBDIR)/rt
tgtincs_offlineobs := offlinedb/include offlineclient
tgtincs_offlineobs += epics/include epics/include/os/Linux
tgtincs_offlineobs += pdsdata/include

tgtsrcs_common := Handler.cc ShmClient.cc
tgtlibs_common := pds/epicstools epics/ca epics/Com pdsdata/appdata pdsdata/compressdata
#tgtsrcs_common := Handler.cc ShmClient.cc EpicsCA.cc
#tgtlibs_common := pds/epicstools epics/ca epics/Com

tgtsrcs_sxrmon := sxrmon.cc SxrSpectrum.cc IpimbHandler.cc $(tgtsrcs_common)
tgtlibs_sxrmon := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/appdata
tgtlibs_sxrmon += pds/service
tgtlibs_sxrmon += $(tgtlibs_common)
tgtslib_sxrmon := $(USRLIBDIR)/rt
tgtincs_sxrmon := epics/include epics/include/os/Linux
tgtincs_sxrmon += pdsdata/include ndarray/include boost/include 

tgtsrcs_xppmon := xppmon.cc EpicsToEpics.cc CspadMon.cc BldIpm.cc XppIpm.cc XppPim.cc Encoder.cc $(tgtsrcs_common)
tgtlibs_xppmon := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/appdata
tgtlibs_xppmon += pds/service
tgtlibs_xppmon += $(tgtlibs_common)
tgtslib_xppmon := $(USRLIBDIR)/rt
tgtincs_xppmon := epics/include epics/include/os/Linux
tgtincs_xppmon += pdsdata/include ndarray/include boost/include   

tgtsrcs_cximon := cximon.cc CspadMon.cc CxiSpectrum.cc $(tgtsrcs_common)
tgtlibs_cximon := pdsdata/xtcdata pdsdata/appdata
tgtlibs_cximon += pdsdata/psddl_pdsdata
tgtlibs_cximon += pds/service
tgtlibs_cximon += $(tgtlibs_common)
tgtslib_cximon := $(USRLIBDIR)/rt
tgtincs_cximon := epics/include epics/include/os/Linux
tgtincs_cximon += pdsdata/include ndarray/include boost/include 

tgtsrcs_xcsmon := xcsmon.cc CspadMon.cc PrincetonMon.cc $(tgtsrcs_common)
tgtlibs_xcsmon := pdsdata/xtcdata pdsdata/appdata
tgtlibs_xcsmon += pdsdata/psddl_pdsdata
tgtlibs_xcsmon += pds/service
tgtlibs_xcsmon += $(tgtlibs_common)
tgtslib_xcsmon := $(USRLIBDIR)/rt
tgtincs_xcsmon := epics/include epics/include/os/Linux
tgtincs_xcsmon += pdsdata/include ndarray/include boost/include 

tgtsrcs_cspadmon := cspadmon.cc CspadMon.cc $(tgtsrcs_common)
tgtlibs_cspadmon := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/appdata
tgtlibs_cspadmon += pds/service
tgtlibs_cspadmon += $(tgtlibs_common)
tgtslib_cspadmon := $(USRLIBDIR)/rt
tgtincs_cspadmon := epics/include epics/include/os/Linux
tgtincs_cspadmon += pdsdata/include ndarray/include boost/include  

tgtsrcs_alive_mon := alive_mon.cc
tgtlibs_alive_mon := pdsdata/xtcdata pdsdata/appdata
tgtlibs_alive_mon += pds/service
tgtslib_alive_mon := $(USRLIBDIR)/rt
tgtincs_alive_mon += pdsdata/include

#
#  LCLS-II development
#
tgtnames := 