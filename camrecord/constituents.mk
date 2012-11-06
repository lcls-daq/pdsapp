tgtnames := camrecord

commonlibs := pdsdata/xtcdata pdsdata/camdata pdsdata/pulnixdata pdsdata/opal1kdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata pdsdata/camdata pdsdata/compressdata
tgtsrcs_camrecord := bld.cc ca.cc main.cc xtc.cc
tgtlibs_camrecord := $(commonlibs) epics/ca epics/Com
tgtincs_camrecord := epics/include epics/include/os/Linux
