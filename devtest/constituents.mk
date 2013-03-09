CPPFLAGS += -D_ACQIRIS -D_LINUX

#tgtnames    := evgr xtcwriter pnccd xtctruncate pnccdreader
tgtnames    := evgr pnccd xtctruncate pnccdreader dsstest acltest

tgtsrcs_evrobs := evrobs.cc
tgtincs_evrobs := evgr
tgtlibs_evrobs := pdsdata/xtcdata evgr/evr evgr/evg pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr 
tgtslib_evrobs := $(USRLIB)/rt

tgtsrcs_evgr := evgr.cc
tgtincs_evgr := evgr
tgtlibs_evgr := pdsdata/xtcdata pdsdata/evrdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evgr := $(USRLIB)/rt

tgtsrcs_evgrd := evgrd.cc
tgtincs_evgrd := evgr
tgtlibs_evgrd := pdsdata/xtcdata pdsdata/evrdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr
tgtslib_evgrd := $(USRLIB)/rt

tgtsrcs_xtcwriter := xtcwriter.cc
tgtlibs_xtcwriter := pdsdata/xtcdata pdsdata/acqdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility
tgtslib_xtcwriter := $(USRLIB)/rt

tgtsrcs_pnccdreader := pnccdreader.cc
tgtlibs_pnccdreader := pdsdata/pnccddata pdsdata/xtcdata pds/service
tgtslib_pnccdreader := $(USRLIB)/rt

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := pdsdata/xtcdata pds/service
tgtslib_pnccd := $(USRLIB)/rt

tgtsrcs_xtctruncate := xtctruncate.cc
tgtlibs_xtctruncate := pdsdata/xtcdata pds/service
tgtslib_xtctruncate := $(USRLIB)/rt

tgtsrcs_dsstest := dsstest.cc
tgtlibs_dsstest := pds/service
tgtslib_dsstest := $(USRLIB)/rt

tgtsrcs_acltest := acltest.cc
tgtslib_acltest := $(USRLIB)/rt $(USRLIB)/acl