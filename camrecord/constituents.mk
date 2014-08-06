tgtnames := camrecord xtcmerge clockadjust

commonlibs := pdsdata/indexdata pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata
tgtsrcs_camrecord := bld.cc ca.cc main.cc xtc.cc
tgtlibs_camrecord := $(commonlibs) offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com pds/service pdsdata/xtcdata
tgtincs_camrecord := offlinedb/include epics/include epics/include/os/Linux pdsdata/include ndarray/include boost/include  

tgtsrcs_xtcmerge := xtcmerge.cc
tgtlibs_xtcmerge := pdsdata/xtcdata
tgtincs_xtcmerge := pdsdata/include

tgtsrcs_clockadjust := clockadjust.cc
tgtlibs_clockadjust := pdsdata/xtcdata
tgtincs_clockadjust := pdsdata/include
