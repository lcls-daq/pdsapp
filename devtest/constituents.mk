CPPFLAGS += -D_ACQIRIS -D_LINUX

libnames    := simframe simmovie simtimetool playframe acqsim epixsim

libsrcs_simframe := SimFrame.cc
liblibs_simframe := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_simframe += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_simframe := pdsdata/include ndarray/include boost/include 

libsrcs_simmovie := SimMovie.cc
liblibs_simmovie := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_simmovie += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_simmovie := pdsdata/include ndarray/include boost/include 

libsrcs_simtimetool := SimTimeTool.cc
liblibs_simtimetool := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_simtimetool += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_simtimetool := pdsdata/include ndarray/include boost/include 

libsrcs_playframe := PlayFrame.cc
liblibs_playframe := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_playframe += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_playframe := pdsdata/include ndarray/include boost/include 

libsrcs_acqsim := AcqWriter.cc
liblibs_acqsim := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_acqsim += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_acqsim := pdsdata/include ndarray/include boost/include 

libsrcs_epixsim := EpixWriter.cc
liblibs_epixsim := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata 
liblibs_epixsim += pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/client 
libincs_epixsim := pdsdata/include ndarray/include boost/include 

tgtnames    := evgr evg pnccdwriter xtctruncate pnccdreader dsstest xcasttest xtccompress pgpwidget pnccdwidget xtccamfix compressstat epixwriter microspin xtcwriter  epix100abintoxtc

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
tgtnames += pgpaeswidget
endif

tgtsrcs_evrobs := evrobs.cc
tgtincs_evrobs := evgr
tgtlibs_evrobs := pdsdata/xtcdata evgr/evr evgr/evg pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr 
tgtslib_evrobs := $(USRLIB)/rt

tgtsrcs_evg := evg.cc
tgtincs_evg := evg
tgtlibs_evg := pdsdata/xtcdata pdsdata/psddl_pdsdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr pds/configdata pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtslib_evg := $(USRLIBDIR)/rt

tgtsrcs_evgr := evgr.cc
tgtincs_evgr := evgr
tgtlibs_evgr := pdsdata/xtcdata pdsdata/psddl_pdsdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr pds/configdata pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtslib_evgr := $(USRLIBDIR)/rt 

tgtsrcs_evgrd := evgrd.cc
tgtincs_evgrd := evgr
tgtlibs_evgrd := pdsdata/xtcdata pdsdata/psddl_pdsdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr
tgtslib_evgrd := $(USRLIB)/rt

tgtsrcs_xtcwriter := xtcwriter.cc
tgtlibs_xtcwriter := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility
tgtslib_xtcwriter := $(USRLIB)/rt $(USRLIB)/dl
tgtincs_xtcwriter := pdsdata/include ndarray/include boost/include 

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
tgtlibs_dsstest := pds/service pdsdata/xtcdata
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

tgtsrcs_pgpwidget := pgpWidget.cc
tgtlibs_pgpwidget := pds/pgp pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_pgpwidget := $(USRLIB)/rt
tgtincs_pgpwidget := pdsdata/include ndarray/include boost/include

tgtsrcs_pnccdwidget := pnccdWidget.cc
tgtlibs_pnccdwidget := pds/pgp
tgtslib_pnccdwidget := $(USRLIB)/rt
tgtincs_pnccdwidget := pdsdata/include 

tgtsrcs_pgpaeswidget := pgpAesWidget.cc
tgtlibs_pgpaeswidget := pds/pgp pds/pgpv3 pdsapp/padmon pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_pgpaeswidget := $(USRLIB)/rt
tgtincs_pgpaeswidget := pdsdata/include ndarray/include boost/include aesdriver/include

tgtsrcs_fccdwidget := fccdWidget.cc
tgtslib_fccdwidget := $(USRLIB)/rt
tgtincs_fccdwidget := pdsdata/include 

tgtsrcs_xtccamfix := xtccamfix.cc
tgtlibs_xtccamfix := pdsdata/xtcdata
tgtslib_xtccamfix := ${USRLIBDIR}/rt
tgtincs_xtccamfix := pdsdata/include

tgtsrcs_compressstat := compressstat.cc
tgtlibs_compressstat := pds/service pdsdata/xtcdata pdsdata/compressdata pdsdata/anadata pdsdata/indexdata
tgtslib_compressstat := ${USRLIBDIR}/rt ${USRLIBDIR}/pthread 
tgtincs_compressstat := pdsdata/include boost/include

tgtsrcs_fccdmonserver := fccdmonserver.cc
tgtlibs_fccdmonserver := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/udpcam pds/config pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_fccdmonserver += pdsdata/xtcdata pdsdata/compressdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtslib_fccdmonserver := ${USRLIBDIR}/rt
tgtincs_fccdmonserver := pdsdata/include boost/include ndarray/include

tgtsrcs_buffer := buffer.cc
tgtslib_buffer := ${USRLIBDIR}/rt

tgtsrcs_epixbintoxtc := epixbintoxtc.cc
tgtlibs_epixbintoxtc := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/udpcam pds/config pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_epixbintoxtc += pdsdata/xtcdata pdsdata/compressdata pdsdata/psddl_pdsdata
tgtslib_epixbintoxtc := ${USRLIBDIR}/rt
tgtincs_epixbintoxtc := pdsdata/include boost/include ndarray/include

tgtsrcs_epix10kbintoxtc := epix10kbintoxtc.cc
tgtlibs_epix10kbintoxtc := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/udpcam pds/config pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_epix10kbintoxtc += pdsdata/xtcdata pdsdata/compressdata pdsdata/psddl_pdsdata
tgtslib_epix10kbintoxtc := ${USRLIBDIR}/rt
tgtincs_epix10kbintoxtc := pdsdata/include boost/include ndarray/include

tgtsrcs_epix100abintoxtc := epix100abintoxtc.cc
tgtlibs_epix100abintoxtc := pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/udpcam pds/config pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_epix100abintoxtc += pdsdata/xtcdata pdsdata/compressdata pdsdata/psddl_pdsdata
tgtslib_epix100abintoxtc := ${USRLIBDIR}/rt
tgtincs_epix100abintoxtc := pdsdata/include boost/include ndarray/include

tgtsrcs_netlink := netlink.cc
tgtlibs_netlink := pds/collection pds/service pdsdata/xtcdata
tgtslib_netlink := ${USRLIBDIR}/rt
tgtincs_netlink := 

tgtsrcs_quadadc_mon := quadadc_mon.cc
tgtlibs_quadadc_mon := pds/service 
tgtlibs_quadadc_mon += hsd/hsd
tgtlibs_quadadc_mon += pdsdata/xtcdata pdsdata/psddl_pdsdata pdsapp/padmon pdsdata/appdata
tgtincs_quadadc_mon := pdsdata/include ndarray/include boost/include evgr
tgtslib_quadadc_mon := $(USRLIBDIR)/rt pthread
tgtincs_quadadc_mon += hsd/include

tgtnames += quadadc_mon

tgtsrcs_quadadc_xtc := quadadc_xtc.cc
tgtlibs_quadadc_xtc += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtincs_quadadc_xtc := pdsdata/include ndarray/include boost/include
tgtslib_quadadc_xtc := $(USRLIBDIR)/rt

tgtnames += quadadc_xtc

libnames += promload
libsrcs_promload := McsFile.cc AxiMicronN25Q.cc AxiCypressS25.cc MMDevice.cc PgpDevice.cc 
liblibs_promload := pds/pgpv3

tgtsrcs_pgppromload := pgppromload.cc
tgtlibs_pgppromload := pdsapp/promload pds/pgpv3
tgtslib_pgppromload := $(USRLIBDIR)/rt
tgtincs_pgppromload := pgpcard aesdriver/include

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
tgtnames += pgppromload
endif
