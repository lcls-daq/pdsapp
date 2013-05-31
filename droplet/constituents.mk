tgtnames := droplet
LXFLAGS += -Wl,--export-dynamic
commonlibs := pdsdata/xtcdata pdsdata/pulnixdata pdsdata/opal1kdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata pdsdata/camdata pdsdata/compressdata pdsdata/lusidata

tgtsrcs_droplet := droplet.cc
tgtlibs_droplet := $(commonlibs)
tgtslib_droplet := dl
tgtincs_droplet := epics/include epics/include/os/Linux
