tgtnames    := xtcEpicsWriterTest

tgtsrcs_xtcEpicsWriterTest := xtcEpicsWriterTest.cc XtcEpicsMonitor.cc XtcEpicsMonitor.hh console_io.cc console_io.hh
tgtincs_xtcEpicsWriterTest := epics/include epics/include/os/Linux pds/epicsArch
tgtlibs_xtcEpicsWriterTest := pdsdata/xtcdata pdsdata/epics pds/service pds/epicsArch pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config epics/ca epics/Com pdsdata/aliasdata
tgtslib_xtcEpicsWriterTest := 
