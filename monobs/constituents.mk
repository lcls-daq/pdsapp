libnames := 

tgtnames := monobs monshm

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/acqdata

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := /usr/lib/rt

tgtsrcs_monshm := monshm.cc CamDisplay.cc AcqDisplay.cc XtcMonitorClient.cc
tgtlibs_monshm := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata 
tgtlibs_monshm += $(commonlibs) pds/mon
tgtslib_monshm := /usr/lib/rt
