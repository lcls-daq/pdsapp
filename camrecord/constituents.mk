tgtnames := camrecord xtcmerge clockadjust dropcheck undtest

commonlibs := pdsdata/indexdata pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata
tgtsrcs_camrecord := bld.cc ca.cc main.cc xtc.cc hdf5.cc
tgtlibs_camrecord := $(commonlibs) offlinedb/mysqlclient offlinedb/offlinedb epics/ca epics/Com pds/service szip/sz hdf5/hdf5
tgtlibs_camrecord += pds/logbookclient python3/python3.6m

tgtincs_camrecord := offlinedb/include epics/include epics/include/os/Linux pdsdata/include ndarray/include boost/include hdf5/include python3/include/python3.6m
tgtslib_camrecord := $(USRLIBDIR)/pthread

tgtsrcs_xtcmerge := xtcmerge.cc
tgtlibs_xtcmerge := pdsdata/xtcdata
tgtincs_xtcmerge := pdsdata/include

tgtsrcs_clockadjust := clockadjust.cc
tgtlibs_clockadjust := pdsdata/xtcdata
tgtincs_clockadjust := pdsdata/include

tgtsrcs_dropcheck := dropcheck.cc
tgtlibs_dropcheck := $(commonlibs)
tgtincs_dropcheck := pdsdata/include ndarray/include boost/include

tgtsrcs_undtest := undtest.cc
tgtlibs_undtest := $(commonlibs) epics/ca epics/Com
tgtincs_undtest := epics/include epics/include/os/Linux
