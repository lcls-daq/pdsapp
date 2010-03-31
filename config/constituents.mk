libnames       := configdb
libsrcs_configdb := Path.cc \
        Table.cc \
        Device.cc \
        DeviceEntry.cc \
        Experiment.cc \
        PdsDefs.cc
libsrcs_configdb += ControlScan.cc ControlScan_moc.cc
libsrcs_configdb += Reconfig_Ui.cc Reconfig_Ui_moc.cc
libsrcs_configdb += Dialog.cc     Dialog_moc.cc 
libsrcs_configdb += SubDialog.cc    SubDialog_moc.cc 
libsrcs_configdb += Validators.cc     Validators_moc.cc 
libsrcs_configdb += ParameterSet.cc   ParameterSet_moc.cc
libsrcs_configdb += SerializerDictionary.cc
libsrcs_configdb += Serializer.cc
libsrcs_configdb += Opal1kConfig.cc
libsrcs_configdb += pnCCDConfig.cc
libsrcs_configdb += princetonConfig.cc
libsrcs_configdb += TM6740Config.cc
libsrcs_configdb += FrameFexConfig.cc
libsrcs_configdb += EvrOutputMap.cc EvrPulseConfig.cc EvrEventCode.cc
libsrcs_configdb += EvrConfig.cc EvrConfig_V1.cc
libsrcs_configdb += ControlConfig.cc
libsrcs_configdb += IpimbConfig.cc
libsrcs_configdb += AcqConfig.cc
libsrcs_configdb += Parameters.cc
libsrcs_configdb += BitCount.cc
libsrcs_configdb += templates.cc
libincs_configdb := qt/include

tgtnames       := configdb
tgtnames       += configdb_gui
tgtnames       += configdb_list
#tgtnames       += create_scan

# executable python modules: configdb_gui.py

tgtsrcs_configdb := configdb.cc
tgtincs_configdb := qt/include
tgtlibs_configdb := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata 
tgtlibs_configdb += pdsapp/configdb
tgtlibs_configdb += qt/QtGui qt/QtCore
tgtslib_configdb := /usr/lib/rt

tgtsrcs_configdb_gui := configdb_gui.cc
tgtsrcs_configdb_gui += Ui.cc
tgtsrcs_configdb_gui += Devices_Ui.cc     Devices_Ui_moc.cc
tgtsrcs_configdb_gui += Experiment_Ui.cc  Experiment_Ui_moc.cc
tgtsrcs_configdb_gui += Transaction_Ui.cc   Transaction_Ui_moc.cc
tgtsrcs_configdb_gui += Info_Ui.cc    Info_Ui_moc.cc
tgtsrcs_configdb_gui += ListUi.cc     ListUi_moc.cc
tgtsrcs_configdb_gui += DetInfoDialog_Ui.cc   DetInfoDialog_Ui_moc.cc
tgtincs_configdb_gui := qt/include
tgtlibs_configdb_gui := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata 
tgtlibs_configdb_gui += qt/QtGui qt/QtCore
tgtlibs_configdb_gui += pdsapp/configdb
tgtslib_configdb_gui := /usr/lib/rt

tgtsrcs_configdb_list := configdb_list.cc
tgtsrcs_configdb_list += ListUi.cc ListUi_moc.cc
tgtincs_configdb_list := qt/include
tgtlibs_configdb_list := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata 
tgtlibs_configdb_list += qt/QtGui qt/QtCore
tgtlibs_configdb_list += pdsapp/configdb
tgtslib_configdb_list := /usr/lib/rt

tgtsrcs_create_scan := create_scan_config.cc
tgtincs_create_scan := qt/include
tgtlibs_create_scan := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata 
tgtlibs_create_scan += qt/QtGui qt/QtCore
tgtlibs_create_scan += pdsapp/configdb
tgtslib_create_scan := /usr/lib/rt
