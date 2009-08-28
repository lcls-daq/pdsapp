tgtnames    := xtcEpicsWriterTest

tgtsrcs_xtcEpicsWriterTest := xtcEpicsWriterTest.cc EpicsMonitorPv.cc EpicsMonitorPv.hh XtcEpicsMonitor.cc XtcEpicsMonitor.hh XtcEpicsPv.cc XtcEpicsPv.hh console_io.cc console_io.hh
tgtincs_xtcEpicsWriterTest := epics/include epics/include/os/Linux
tgtlibs_xtcEpicsWriterTest := pdsdata/xtcdata pdsdata/epics pds/service epics/ca epics/Com
tgtslib_xtcEpicsWriterTest := 
