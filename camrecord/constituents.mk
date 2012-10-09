tgtnames := camrecord

commonlibs := pdsdata/xtcdata pdsdata/camdata pdsdata/pulnixdata pdsdata/opal1kdata
tgtsrcs_camrecord := bld.cc ca.cc main.cc xtc.cc
tgtlibs_camrecord := $(commonlibs) epics/ca epics/Com
tgtincs_camrecord := epics/include epics/include/os/Linux
