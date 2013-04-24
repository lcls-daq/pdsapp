tgtnames := control_gui catest

#  Get datalibs macro
include ../../pdsdata/packages.mk

tgtsrcs_control_gui := control.cc 
tgtsrcs_control_gui += MainWindow.cc      MainWindow_moc.cc
tgtsrcs_control_gui += ConfigSelect.cc    ConfigSelect_moc.cc
tgtsrcs_control_gui += NodeSelect.cc      NodeSelect_moc.cc
tgtsrcs_control_gui += SelectDialog.cc    SelectDialog_moc.cc
tgtsrcs_control_gui += PartitionSelect.cc PartitionSelect_moc.cc
tgtsrcs_control_gui += StateSelect.cc     StateSelect_moc.cc
tgtsrcs_control_gui += PVDisplay.cc       PVDisplay_moc.cc
tgtsrcs_control_gui += ControlLog.cc    ControlLog_moc.cc
tgtsrcs_control_gui += DamageStats.cc     DamageStats_moc.cc
tgtsrcs_control_gui += RunStatus.cc     RunStatus_moc.cc
tgtsrcs_control_gui += MySqlRunAllocator.cc
tgtsrcs_control_gui += FileRunAllocator.cc
tgtsrcs_control_gui += SeqAppliance.cc
tgtsrcs_control_gui += RemoteSeqApp.cc
tgtsrcs_control_gui += PVManager.cc
tgtsrcs_control_gui += PVMonitor.cc
tgtsrcs_control_gui += PVControl.cc
#tgtsrcs_control_gui += EpicsCA.cc
tgtsrcs_control_gui += EventSequencer.cc
tgtlibs_control_gui := $(datalibs)
tgtlibs_control_gui += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/client pds/offlineclient pds/config pds/management pds/epicstools
tgtlibs_control_gui += pds/epicstools
tgtlibs_control_gui += pdsapp/configdb
tgtlibs_control_gui += pdsapp/configdbg
tgtlibs_control_gui += $(qtlibdir)
tgtlibs_control_gui += epics/ca epics/Com
tgtlibs_control_gui += offlinedb/mysqlclient offlinedb/offlinedb
tgtslib_control_gui := $(USRLIBDIR)/rt $(qtslibdir)
tgtincs_control_gui := $(qtincdir)
tgtincs_control_gui += epics/include epics/include/os/Linux
tgtincs_control_gui += offlinedb/include

tgtsrcs_catest := catest.cc PVMonitor.cc
tgtslib_catest := $(USRLIBDIR)/rt
tgtincs_catest := epics/include epics/include/os/Linux
tgtlibs_catest := epics/ca epics/Com
tgtlibs_catest += pdsdata/controldata pdsdata/xtcdata
tgtlibs_catest += pds/service pds/mon pds/vmon pds/collection pds/xtc pds/utility pds/config
tgtlibs_catest += pds/epicstools
