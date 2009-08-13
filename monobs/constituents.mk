libnames := 

tgtnames := monobs monshmserver

commonlibs := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/acqdata

tgtsrcs_monobs := monobs.cc CamDisplay.cc AcqDisplay.cc
tgtlibs_monobs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata
tgtlibs_monobs += $(commonlibs) pds/mon
tgtslib_monobs := /usr/lib/rt

tgtsrcs_monshmserver := monshmserver.cc
tgtlibs_monshmserver := $(commonlibs) pds/mon
tgtslib_monshmserver := /usr/lib/rt
