# constituents for the segment level device production code ONLY

CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := evr evrstandalone evrsnoop acq opal1k epicsArch rceProxy encoder bld princeton ipimb lusidiag tm6740 pimimage fccd

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := pdsdata/xtcdata pdsdata/acqdata acqiris/AqDrv4 pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/acqiris pds/management pds/client pds/config 
tgtslib_acq := /usr/lib/rt

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb
tgtlibs_ipimb := pdsdata/xtcdata pdsdata/ipimbdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/ipimb pds/management pds/client pds/config 
tgtslib_ipimb := /usr/lib/rt

tgtsrcs_lusidiag := lusidiag.cc
tgtincs_lusidiag := lusidiag
tgtlibs_lusidiag := pdsdata/xtcdata pdsdata/ipimbdata pdsdata/lusidata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/ipimb pds/management pds/client pds/config 
tgtslib_lusidiag := /usr/lib/rt

tgtsrcs_encoder := encoder.cc
tgtincs_encoder := encoder
tgtlibs_encoder := pdsdata/xtcdata pdsdata/encoderdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/encoder pds/management pds/client pds/config 
tgtslib_encoder := /usr/lib/rt

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr
tgtlibs_evr := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evr := /usr/lib/rt

tgtsrcs_evrstandalone := evrstandalone.cc
tgtincs_evrstandalone := evgr
tgtlibs_evrstandalone := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evrstandalone += evgr/evr evgr/evg 
tgtlibs_evrstandalone += pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evrstandalone := /usr/lib/rt

tgtsrcs_evrsnoop := evrsnoop.cc
tgtincs_evrsnoop := evgr
tgtlibs_evrsnoop := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evrsnoop += evgr/evr evgr/evg 
tgtlibs_evrsnoop += pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evrsnoop := /usr/lib/rt

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_opal1k += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_opal1k += $(leutron_libs)
tgtincs_opal1k := leutron/include

tgtsrcs_tm6740 := tm6740.cc 
tgtlibs_tm6740 := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_tm6740 += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_tm6740 += $(leutron_libs)
tgtincs_tm6740 := leutron/include

tgtsrcs_pimimage := pimimage.cc 
tgtlibs_pimimage := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_pimimage += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_pimimage += $(leutron_libs)
tgtincs_pimimage := leutron/include

tgtsrcs_fccd := fccd.cc
tgtlibs_fccd := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_fccd += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_fccd += $(leutron_libs)
tgtincs_fccd := leutron/include

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := pdsdata/xtcdata pdsdata/epics pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := /usr/lib/rt

tgtsrcs_bld := bld.cc 
tgtlibs_bld := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_bld += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
tgtslib_bld := /usr/lib/rt

tgtsrcs_rceProxy := rceProxy.cc
tgtlibs_rceProxy := pdsdata/pnccddata pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pds/rceProxy
tgtslib_rceProxy := /usr/lib/rt

tgtsrcs_princeton := princeton.cc
tgtlibs_princeton := pdsdata/pnccddata pdsdata/xtcdata pdsdata/evrdata pdsdata/princetondata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pds/princeton pvcam/pvcam
#tgtlibs_princeton := pdsdata/pnccddata pdsdata/xtcdata pdsdata/evrdata pdsdata/princetondata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pds/princeton pvcam/pvcamtest
tgtslib_princeton := /usr/lib/rt dl pthread
