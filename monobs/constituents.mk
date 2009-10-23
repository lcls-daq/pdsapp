libnames := 

tgtnames := monobs monshm monshmserver offlineobs

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/acqdata

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := /usr/lib/rt

tgtsrcs_monshm := monshm.cc CamDisplay.cc AcqDisplay.cc XtcMonitorClient.cc
tgtlibs_monshm := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata 
tgtlibs_monshm += $(commonlibs) pds/mon
tgtslib_monshm := /usr/lib/rt

tgtsrcs_monshmserver := monshmserver.cc
tgtlibs_monshmserver := $(commonlibs) pds/mon
tgtslib_monshmserver := /usr/lib/rt

tgtsrcs_offlineobs := offlineobs.cc OfflineAppliance.cc
tgtlibs_offlineobs := $(commonlibs) pds/mon pds/offlineclient
tgtlibs_offlineobs += offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com
tgtslib_offlineobs := /usr/lib/rt
tgtincs_offlineobs := offlinedb/include offlineclient
tgtincs_offlineobs += epics/include epics/include/os/Linux

