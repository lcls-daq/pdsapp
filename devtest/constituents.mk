CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := evgr xtcwriter pnccd xtctruncate xtcEpicsWriterTest xtcEpicsReaderTest

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

tgtsrcs_xtcwriter := xtcwriter.cc
tgtlibs_xtcwriter := pdsdata/xtcdata pds/service
tgtslib_xtcwriter := /usr/lib/rt

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := pdsdata/xtcdata pds/service
tgtslib_pnccd := /usr/lib/rt

tgtsrcs_xtctruncate := xtctruncate.cc
tgtlibs_xtctruncate := pdsdata/xtcdata pds/service
tgtslib_xtctruncate := /usr/lib/rt

tgtsrcs_xtcEpicsWriterTest := xtcEpicsWriterTest.cc EpicsMonitorPv.cc EpicsMonitorPv.hh XtcEpicsMonitor.cc XtcEpicsMonitor.hh XtcEpicsPv.cc XtcEpicsPv.hh EpicsPvData.cc EpicsPvData.hh EpicsDbrTools.cc EpicsDbrTools.hh console_io.cc console_io.hh
tgtincs_xtcEpicsWriterTest := epics/include epics/include/os/Linux
tgtlibs_xtcEpicsWriterTest := pdsdata/xtcdata pds/service epics/ca epics/Com
tgtslib_xtcEpicsWriterTest := 

tgtsrcs_xtcEpicsReaderTest := xtcEpicsReaderTest.cc XtcEpicsFileReader.cc XtcEpicsFileReader.hh XtcEpicsIterator.cc XtcEpicsIterator.hh EpicsDbrTools.cc EpicsDbrTools.hh XtcEpicsPv.hh EpicsPvData.cc
tgtincs_xtcEpicsReaderTest := epics/include epics/include/os/Linux
tgtlibs_xtcEpicsReaderTest := pdsdata/xtcdata
tgtslib_xtcEpicsReaderTest := 
