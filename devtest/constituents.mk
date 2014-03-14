CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := evgr pnccdwriter xtctruncate pnccdreader dsstest xcasttest xtccompress pgpwidget pnccdwidget fccdwidget xtccamfix compressstat epixwriter microspin

tgtsrcs_evrobs := evrobs.cc
tgtincs_evrobs := evgr
tgtlibs_evrobs := pdsdata/xtcdata evgr/evr evgr/evg pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr 
tgtslib_evrobs := $(USRLIB)/rt

tgtsrcs_evgr := evgr.cc
tgtincs_evgr := evgr
tgtlibs_evgr := pdsdata/xtcdata pdsdata/psddl_pdsdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr pds/configdata pds/configdbc pds/confignfs pds/configsql
tgtslib_evgr := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_evgrd := evgrd.cc
tgtincs_evgrd := evgr
tgtlibs_evgrd := pdsdata/xtcdata pdsdata/psddl_pdsdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr
tgtslib_evgrd := $(USRLIB)/rt

tgtsrcs_xtcwriter := xtcwriter.cc
tgtlibs_xtcwriter := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility
tgtslib_xtcwriter := $(USRLIB)/rt

tgtsrcs_epixwriter := epixwriter.cc
tgtlibs_epixwriter := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility
tgtslib_epixwriter := $(USRLIB)/rt
tgtincs_epixwriter := pdsdata/include ndarray/include boost/include

tgtsrcs_pnccdreader := pnccdreader.cc
tgtlibs_pnccdreader := pdsdata/psddl_pdsdata pdsdata/xtcdata pds/service
tgtslib_pnccdreader := $(USRLIB)/rt
tgtincs_pnccdreader := pdsdata/include ndarray/include boost/include 

tgtsrcs_pnccdwriter := pnccd.cc
tgtlibs_pnccdwriter := pdsdata/xtcdata pds/service
tgtslib_pnccdwriter := $(USRLIB)/rt
tgtincs_pnccdwriter := pdsdata/include ndarray/include boost/include 

tgtsrcs_xtctruncate := xtctruncate.cc
tgtlibs_xtctruncate := pdsdata/xtcdata pds/service
tgtslib_xtctruncate := $(USRLIB)/rt
tgtincs_xtctruncate := pdsdata/include ndarray/include boost/include 

tgtsrcs_dsstest := dsstest.cc
tgtlibs_dsstest := pds/service
tgtslib_dsstest := $(USRLIB)/rt

tgtsrcs_xcasttest := xcasttest.cc
tgtslib_xcasttest := $(USRLIB)/rt

tgtsrcs_acltest := acltest.cc
tgtslib_acltest := $(USRLIB)/rt $(USRLIB)/acl

tgtsrcs_xcasttest := xcasttest.cc
tgtslib_xcasttest := $(USRLIB)/rt

tgtsrcs_microspin := microspin.cc
tgtslib_microspin := $(USRLIB)/rt

tgtsrcs_xtccompress := xtccompress.cc
tgtlibs_xtccompress := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
tgtlibs_xtccompress += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
tgtlibs_xtccompress += pds/clientcompress pds/pnccdFrameV0 pds/vmon pds/management
tgtslib_xtccompress := ${USRLIBDIR}/rt ${USRLIBDIR}/pthread 
tgtincs_xtccompress := pdsdata/include ndarray/include boost/include 
#CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
DEFINES += -fopenmp

tgtsrcs_pgpwidget := pgpWidget.cc
tgtlibs_pgpwidget := pds/pgp pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_pgpwidget := $(USRLIB)/rt
tgtincs_pgpwidget := pdsdata/include ndarray/include boost/include

tgtsrcs_pnccdwidget := pnccdWidget.cc
tgtlibs_pnccdwidget := pds/pgp
tgtslib_pnccdwidget := $(USRLIB)/rt
tgtincs_pnccdwidget := pdsdata/include 

tgtsrcs_fccdwidget := fccdWidget.cc
tgtslib_fccdwidget := $(USRLIB)/rt
tgtincs_fccdwidget := pdsdata/include 

tgtsrcs_xtccamfix := xtccamfix.cc
tgtlibs_xtccamfix := pdsdata/xtcdata
tgtslib_xtccamfix := ${USRLIBDIR}/rt
tgtincs_xtccamfix := pdsdata/include

tgtsrcs_compressstat := compressstat.cc
tgtlibs_compressstat := pdsdata/xtcdata pdsdata/compressdata pdsdata/anadata pdsdata/indexdata
#tgtslib_compressstat := ${USRLIBDIR}/rt ${USRLIBDIR}/pthread 
tgtincs_compressstat := pdsdata/include boost/include

