ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames := monobs monshm monshmserver sxrmon xppmon
else
tgtnames := monobs monshm monshmserver offlineobs sxrmon xppmon
endif

libnames := 

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/acqdata

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := $(USRLIBDIR)/rt

tgtsrcs_monshm := monshm.cc CamDisplay.cc AcqDisplay.cc XtcMonitorClient.cc
tgtlibs_monshm := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata 
tgtlibs_monshm += $(commonlibs) pds/mon
tgtslib_monshm := $(USRLIBDIR)/rt

tgtsrcs_monshmserver := monshmserver.cc
tgtlibs_monshmserver := pdsdata/appdata pdsdata/pnccddata 
tgtlibs_monshmserver += $(commonlibs) pds/mon pdsapp/tools
tgtslib_monshmserver := $(USRLIBDIR)/rt

tgtsrcs_offlineobs := offlineobs.cc OfflineAppliance.cc
tgtlibs_offlineobs := $(commonlibs) pds/mon pds/offlineclient
tgtlibs_offlineobs += offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com
tgtslib_offlineobs := $(USRLIBDIR)/rt
tgtincs_offlineobs := offlinedb/include offlineclient
tgtincs_offlineobs += epics/include epics/include/os/Linux

tgtsrcs_common := Handler.cc ShmClient.cc EpicsCA.cc 
tgtsrcs_sxrmon := sxrmon.cc SxrSpectrum.cc $(tgtsrcs_common)
tgtlibs_sxrmon := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/appdata
tgtlibs_sxrmon += pds/service
tgtlibs_sxrmon += epics/ca epics/Com
tgtslib_sxrmon := $(USRLIBDIR)/rt
tgtincs_sxrmon := epics/include epics/include/os/Linux

tgtsrcs_xppmon := xppmon.cc XppIpm.cc XppPim.cc $(tgtsrcs_common)
tgtlibs_xppmon := pdsdata/xtcdata pdsdata/ipimbdata pdsdata/appdata
tgtlibs_xppmon += pds/service
tgtlibs_xppmon += epics/ca epics/Com
tgtslib_xppmon := $(USRLIBDIR)/rt
tgtincs_xppmon := epics/include epics/include/os/Linux
