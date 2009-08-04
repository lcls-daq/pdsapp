CPPFLAGS += -D_ACQIRIS -D_LINUX

#tgtnames    := evr evgr acq opal1k xtcwriter pnccd xtctruncate
tgtnames    := evr evgr xtcwriter pnccd xtctruncate xtcEpicsTest
#tgtnames    := evr evgr acq acqevr opal1k xtcwriter pnccd xtctruncate
#tgtnames    := xtcwriter pnccd xtctruncate

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

tgtsrcs_evrobs := evrobs.cc
tgtincs_evrobs := evgr
tgtlibs_evrobs := pdsdata/xtcdata evgr/evr evgr/evg pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr 
tgtslib_evrobs := /usr/lib/rt

tgtsrcs_evgr := evgr.cc
tgtincs_evgr := evgr
tgtlibs_evgr := pdsdata/xtcdata pdsdata/evrdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evgr := /usr/lib/rt

tgtsrcs_evgrd := evgrd.cc
tgtincs_evgrd := evgr
tgtlibs_evgrd := pdsdata/xtcdata pdsdata/evrdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr
tgtslib_evgrd := /usr/lib/rt


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

tgtsrcs_xtcwriter := xtcwriter.cc
tgtlibs_xtcwriter := pdsdata/xtcdata pds/service
tgtslib_xtcwriter := /usr/lib/rt

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := pdsdata/xtcdata pds/service
tgtslib_pnccd := /usr/lib/rt

tgtsrcs_xtctruncate := xtctruncate.cc
tgtlibs_xtctruncate := pdsdata/xtcdata pds/service
tgtslib_xtctruncate := /usr/lib/rt

tgtsrcs_xtcEpicsTest := xtcEpicsTest.cc XtcEpicsFileReader.cc XtcEpicsFileReader.hh XtcEpicsIterator.cc XtcEpicsIterator.hh EpicsDbrTools.cc EpicsDbrTools.hh EpicsMonitorPv.cc EpicsMonitorPv.hh XtcEpicsMonitor.cc XtcEpicsMonitor.hh XtcEpicsPv.cc XtcEpicsPv.hh EpicsPvData.cc EpicsPvData.hh console_io.cc console_io.hh
tgtincs_xtcEpicsTest := epics/include epics/include/os/Linux
tgtlibs_xtcEpicsTest := pdsdata/xtcdata pds/service epics/ca epics/Com
tgtslib_xtcEpicsTest := /usr/lib/rt
