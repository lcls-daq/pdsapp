tgtnames       := configdb
tgtnames       += configdb_gui

# executable python modules: configdb_gui.py

tgtsrcs_configdb := configdb.cc
tgtsrcs_configdb += Table.cc
tgtsrcs_configdb += Device.cc
tgtsrcs_configdb += Experiment.cc
tgtsrcs_configdb += PdsDefs.cc
tgtincs_configdb := qt/include
tgtlibs_configdb := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata pdsdata/acqdata
tgtlibs_configdb += qt/QtGui qt/QtCore
tgtslib_configdb := /usr/lib/rt

tgtsrcs_configdb_gui := configdb_gui.cc
tgtsrcs_configdb_gui += Ui.cc
tgtsrcs_configdb_gui += Devices_Ui.cc Devices_Ui_moc.cc
tgtsrcs_configdb_gui += Experiment_Ui.cc Experiment_Ui_moc.cc
tgtsrcs_configdb_gui += Transaction_Ui.cc Transaction_Ui_moc.cc
tgtsrcs_configdb_gui += DetInfoDialog_Ui.cc DetInfoDialog_Ui_moc.cc
tgtsrcs_configdb_gui += Table.cc
tgtsrcs_configdb_gui += Device.cc
tgtsrcs_configdb_gui += Experiment.cc
tgtsrcs_configdb_gui += PdsDefs.cc
tgtsrcs_configdb_gui += Dialog.cc Dialog_moc.cc 
tgtsrcs_configdb_gui += SubDialog.cc SubDialog_moc.cc 
tgtsrcs_configdb_gui += Validators.cc Validators_moc.cc 
tgtsrcs_configdb_gui += ParameterSet.cc ParameterSet_moc.cc
tgtsrcs_configdb_gui += SerializerDictionary.cc
tgtsrcs_configdb_gui += Opal1kConfig.cc
tgtsrcs_configdb_gui += TM6740Config.cc
tgtsrcs_configdb_gui += FrameFexConfig.cc
tgtsrcs_configdb_gui += EvrConfig.cc
tgtsrcs_configdb_gui += AcqConfig.cc
tgtsrcs_configdb_gui += Parameters.cc
tgtsrcs_configdb_gui += templates.cc
tgtincs_configdb_gui := qt/include
tgtlibs_configdb_gui := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata pdsdata/acqdata
tgtlibs_configdb_gui += qt/QtGui qt/QtCore
tgtslib_configdb_gui := /usr/lib/rt

