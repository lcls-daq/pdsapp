tgtnames       := configtc

# executable python modules: configdb_gui.py

tgtsrcs_configtc := configtc.cc 
tgtsrcs_configtc += ConfigTC_Gui.cc ConfigTC_Gui_moc.cc 
tgtsrcs_configtc += ConfigTC_Dialog.cc ConfigTC_Dialog_moc.cc 
tgtsrcs_configtc += ConfigTC_SubDialog.cc ConfigTC_SubDialog_moc.cc 
tgtsrcs_configtc += ConfigTC_Validators.cc ConfigTC_Validators_moc.cc 
tgtsrcs_configtc += ConfigTC_ParameterSet.cc ConfigTC_ParameterSet_moc.cc
tgtsrcs_configtc += ConfigTC_Opal1kConfig.cc
tgtsrcs_configtc += ConfigTC_FrameFexConfig.cc
tgtsrcs_configtc += ConfigTC_EvrConfig.cc
tgtsrcs_configtc += ConfigTC_Parameters.cc
tgtsrcs_configtc += ConfigTC_templates.cc
tgtsinc_configtc := /pcds/package/qt-4.3.4/include
tgtlibs_configtc := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata pdsdata/evrdata
tgtlibs_configtc += pds/service pds/collection pds/xtc pds/utility pds/config
tgtlibs_configtc += qt/QtGui qt/QtCore
tgtslib_configtc := /usr/lib/rt

