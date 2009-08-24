libnames       := configdb
libsrcs_configdb := Path.cc \
		    Table.cc \
		    Device.cc \
		    DeviceEntry.cc \
		    Experiment.cc \
		    PdsDefs.cc

tgtnames       := configdb
tgtnames       += configdb_gui
tgtnames       += configdb_list

# executable python modules: configdb_gui.py

tgtsrcs_configdb := configdb.cc
tgtincs_configdb := qt/include
tgtlibs_configdb := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata
tgtlibs_configdb += pdsapp/configdb
tgtlibs_configdb += qt/QtGui qt/QtCore
tgtslib_configdb := /usr/lib/rt

tgtsrcs_configdb_gui := configdb_gui.cc
tgtsrcs_configdb_gui += Ui.cc
tgtsrcs_configdb_gui += Devices_Ui.cc Devices_Ui_moc.cc
tgtsrcs_configdb_gui += Experiment_Ui.cc Experiment_Ui_moc.cc
tgtsrcs_configdb_gui += Transaction_Ui.cc Transaction_Ui_moc.cc
tgtsrcs_configdb_gui += DetInfoDialog_Ui.cc DetInfoDialog_Ui_moc.cc
tgtsrcs_configdb_gui += Dialog.cc Dialog_moc.cc 
tgtsrcs_configdb_gui += SubDialog.cc SubDialog_moc.cc 
tgtsrcs_configdb_gui += Validators.cc Validators_moc.cc 
tgtsrcs_configdb_gui += ParameterSet.cc ParameterSet_moc.cc
tgtsrcs_configdb_gui += SerializerDictionary.cc
tgtsrcs_configdb_gui += Serializer.cc
tgtsrcs_configdb_gui += Opal1kConfig.cc
tgtsrcs_configdb_gui += TM6740Config.cc
tgtsrcs_configdb_gui += FrameFexConfig.cc
tgtsrcs_configdb_gui += EvrConfig.cc
tgtsrcs_configdb_gui += ControlConfig.cc
tgtsrcs_configdb_gui += AcqConfig.cc
tgtsrcs_configdb_gui += Parameters.cc
tgtsrcs_configdb_gui += BitCount.cc
tgtsrcs_configdb_gui += templates.cc
tgtincs_configdb_gui := qt/include
tgtlibs_configdb_gui := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata
tgtlibs_configdb_gui += qt/QtGui qt/QtCore
tgtlibs_configdb_gui += pdsapp/configdb
tgtslib_configdb_gui := /usr/lib/rt

tgtsrcs_configdb_list := configdb_list.cc
tgtsrcs_configdb_list += ListUi.cc ListUi_moc.cc
tgtsrcs_configdb_list += Dialog.cc Dialog_moc.cc 
tgtsrcs_configdb_list += SubDialog.cc SubDialog_moc.cc 
tgtsrcs_configdb_list += Validators.cc Validators_moc.cc 
tgtsrcs_configdb_list += ParameterSet.cc ParameterSet_moc.cc
tgtsrcs_configdb_list += SerializerDictionary.cc
tgtsrcs_configdb_list += Serializer.cc
tgtsrcs_configdb_list += Opal1kConfig.cc
tgtsrcs_configdb_list += TM6740Config.cc
tgtsrcs_configdb_list += FrameFexConfig.cc
tgtsrcs_configdb_list += EvrConfig.cc
tgtsrcs_configdb_list += ControlConfig.cc
tgtsrcs_configdb_list += AcqConfig.cc
tgtsrcs_configdb_list += Parameters.cc
tgtsrcs_configdb_list += BitCount.cc
tgtsrcs_configdb_list += templates.cc
tgtincs_configdb_list := qt/include
tgtlibs_configdb_list := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata
tgtlibs_configdb_list += qt/QtGui qt/QtCore
tgtlibs_configdb_list += pdsapp/configdb
tgtslib_configdb_list := /usr/lib/rt

