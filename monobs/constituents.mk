ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames := monobs monshm monshmserver sxrmon xppmon cspadmon
else
tgtnames := monobs monshm monshmserver offlineobs sxrmon xppmon cspadmon alive_mon
endif

libnames := 

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/acqdata
liblibs_tools := pdsdata/cspaddata pdsdata/pnccddata

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := $(USRLIBDIR)/rt

tgtsrcs_monshm := monshm.cc CamDisplay.cc AcqDisplay.cc XtcMonitorClient.cc
tgtlibs_monshm := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata 
tgtlibs_monshm += $(commonlibs) pds/mon
tgtslib_monshm := $(USRLIBDIR)/rt

tgtsrcs_monshmserver := monshmserver.cc
tgtlibs_monshmserver := pdsdata/appdata 
tgtlibs_monshmserver += $(commonlibs) pds/mon $(liblibs_tools) pdsapp/tools pdsdata/indexdata pdsdata/evrdata
tgtslib_monshmserver := $(USRLIBDIR)/rt

tgtsrcs_offlineobs := offlineobs.cc OfflineAppliance.cc
tgtlibs_offlineobs := $(commonlibs) pds/mon pds/offlineclient
tgtlibs_offlineobs += offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com
tgtslib_offlineobs := $(USRLIBDIR)/rt
tgtincs_offlineobs := offlinedb/include offlineclient
tgtincs_offlineobs += epics/include epics/include/os/Linux

tgtsrcs_common := Handler.cc ShmClient.cc
tgtlibs_common := pds/epicstools epics/ca epics/Com
#tgtsrcs_common := Handler.cc ShmClient.cc EpicsCA.cc
#tgtlibs_common := pds/epicstools epics/ca epics/Com

tgtsrcs_sxrmon := sxrmon.cc SxrSpectrum.cc IpimbHandler.cc $(tgtsrcs_common)
tgtlibs_sxrmon := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/ipimbdata pdsdata/appdata
tgtlibs_sxrmon += pds/service
tgtlibs_sxrmon += $(tgtlibs_common)
tgtslib_sxrmon := $(USRLIBDIR)/rt
tgtincs_sxrmon := epics/include epics/include/os/Linux

tgtsrcs_xppmon := xppmon.cc CspadMon.cc BldIpm.cc XppIpm.cc XppPim.cc $(tgtsrcs_common)
tgtlibs_xppmon := pdsdata/xtcdata pdsdata/ipimbdata pdsdata/appdata
tgtlibs_xppmon += pdsdata/evrdata pdsdata/cspaddata
tgtlibs_xppmon += pds/service
tgtlibs_xppmon += $(tgtlibs_common)
tgtslib_xppmon := $(USRLIBDIR)/rt
tgtincs_xppmon := epics/include epics/include/os/Linux

tgtsrcs_cspadmon := cspadmon.cc CspadMon.cc $(tgtsrcs_common)
tgtlibs_cspadmon := pdsdata/xtcdata pdsdata/cspaddata pdsdata/evrdata pdsdata/appdata
tgtlibs_cspadmon += pds/service
tgtlibs_cspadmon += $(tgtlibs_common)
tgtslib_cspadmon := $(USRLIBDIR)/rt
tgtincs_cspadmon := epics/include epics/include/os/Linux

tgtsrcs_alive_mon := alive_mon.cc
tgtlibs_alive_mon := pdsdata/xtcdata pdsdata/appdata
tgtlibs_alive_mon += pds/service
tgtslib_alive_mon := $(USRLIBDIR)/rt
