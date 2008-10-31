CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames          := evgrtest acqtest

tgtsrcs_acqtest := acqtest.cc
tgtincs_acqtest := acqiris evgr
tgtlibs_acqtest := acqiris/AqDrv4 evgr/evr evgr/evg pds/service pds/acqiris pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr
tgtslib_acqtest := /usr/lib/rt

tgtsrcs_evgrtest := evgrtest.cc
tgtincs_evgrtest := evgr
tgtlibs_evgrtest := evgr/evg evgr/evr pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr
tgtslib_evgrtest := /usr/lib/rt
