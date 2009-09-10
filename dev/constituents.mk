# constituents for the segment level device production code ONLY

CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := evr acq opal1k epicsArch # tm6740

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := pdsdata/xtcdata pdsdata/acqdata acqiris/AqDrv4 pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/acqiris pds/management pds/client pds/config 
tgtslib_acq := /usr/lib/rt

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr
tgtlibs_evr := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evr := /usr/lib/rt


#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_opal1k += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_opal1k += $(leutron_libs)
tgtincs_opal1k := leutron/include

tgtsrcs_tm6740 := tm6740.cc 
tgtlibs_tm6740 := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_tm6740 += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_tm6740 += $(leutron_libs)
tgtincs_tm6740 := leutron/include

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := pdsdata/xtcdata pdsdata/epics pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := /usr/lib/rt
