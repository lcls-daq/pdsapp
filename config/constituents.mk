ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
qtincdir  := qt/include64
else
qtincdir  := qt/include
endif

libnames       := configdb
libsrcs_configdb := Path.cc \
        Table.cc \
        Device.cc \
        DeviceEntry.cc \
        Experiment.cc \
        PdsDefs.cc
libsrcs_configdb += ControlScan.cc ControlScan_moc.cc
libsrcs_configdb += PvScan.cc PvScan_moc.cc
libsrcs_configdb += EvrScan.cc EvrScan_moc.cc
libsrcs_configdb += Reconfig_Ui.cc Reconfig_Ui_moc.cc
libsrcs_configdb += Dialog.cc     Dialog_moc.cc 
libsrcs_configdb += SubDialog.cc    SubDialog_moc.cc 
libsrcs_configdb += Validators.cc     Validators_moc.cc 
libsrcs_configdb += ParameterSet.cc   ParameterSet_moc.cc
libsrcs_configdb += QrLabel.cc   QrLabel_moc.cc
libsrcs_configdb += GlobalCfg.cc
libsrcs_configdb += SerializerDictionary.cc
libsrcs_configdb += ExpertDictionary.cc
libsrcs_configdb += Serializer.cc
libsrcs_configdb += Opal1kConfig.cc
libsrcs_configdb += FccdConfig.cc
libsrcs_configdb += pnCCDConfig.cc
libsrcs_configdb += princetonConfig.cc
libsrcs_configdb += TM6740Config.cc
libsrcs_configdb += TM6740ConfigV1.cc
libsrcs_configdb += FrameFexConfig.cc
libsrcs_configdb += EvrOutputMap.cc EvrPulseConfig.cc EvrPulseConfig_V1.cc EvrEventCodeV3.cc EvrEventCode.cc
libsrcs_configdb += EvrIOChannel.cc EvrIOChannel_moc.cc
libsrcs_configdb += EvrIOConfig.cc EvrIOConfig_moc.cc
libsrcs_configdb += PolarityButton.cc
libsrcs_configdb += EvrPulseTable_V4.cc EvrPulseTable_V4_moc.cc
libsrcs_configdb += EventcodeTiming.cc
libsrcs_configdb += EvrPulseTable.cc EvrPulseTable_moc.cc
libsrcs_configdb += EvrEventDesc.cc EvrEventDesc_moc.cc
libsrcs_configdb += EvrSeqEventDesc.cc
libsrcs_configdb += EvrGlbEventDesc.cc
libsrcs_configdb += EvrEventCodeTable.cc EvrEventCodeTable_moc.cc
libsrcs_configdb += EvrConfig.cc EvrConfigP.cc EvrConfig_V4.cc EvrConfig_V3.cc EvrConfig_V2.cc EvrConfig_V1.cc
libsrcs_configdb += SequencerConfig.cc SequencerConfig_moc.cc
libsrcs_configdb += CspadConfigTable_V1.cc 
libsrcs_configdb += CspadConfig_V1.cc
libsrcs_configdb += CspadSector.cc
libsrcs_configdb += CspadGainMap.cc CspadGainMap_moc.cc
libsrcs_configdb += CspadConfigTable.cc CspadConfigTable_moc.cc
libsrcs_configdb += CspadConfig.cc
libsrcs_configdb += ControlConfig.cc
libsrcs_configdb += IpimbConfig.cc
libsrcs_configdb += DiodeFexItem.cc
libsrcs_configdb += IpmFexTable.cc
libsrcs_configdb += IpmFexConfig.cc
libsrcs_configdb += DiodeFexTable.cc
libsrcs_configdb += DiodeFexConfig.cc
libsrcs_configdb += PimImageConfig.cc
libsrcs_configdb += EncoderConfig.cc
libsrcs_configdb += AcqConfig.cc
libsrcs_configdb += AcqTdcConfig.cc
libsrcs_configdb += Parameters.cc
libsrcs_configdb += BitCount.cc
libsrcs_configdb += templates.cc
libincs_configdb := $(qtincdir)

tgtnames       := configdb
tgtnames       += configdb_gui
tgtnames       += configdb_list
#tgtnames       += create_scan

# executable python modules: configdb_gui.py

datalibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata pdsdata/encoderdata pdsdata/fccddata pdsdata/lusidata pdsdata/cspaddata

tgtsrcs_configdb := configdb.cc
tgtincs_configdb := $(qtincdir)
tgtlibs_configdb := $(datalibs)
tgtlibs_configdb += pdsapp/configdb
tgtlibs_configdb += qt/QtGui qt/QtCore
tgtslib_configdb := $(USRLIBDIR)/rt

tgtsrcs_configdb_gui := configdb_gui.cc
tgtsrcs_configdb_gui += Ui.cc
tgtsrcs_configdb_gui += Devices_Ui.cc 		Devices_Ui_moc.cc
tgtsrcs_configdb_gui += Experiment_Ui.cc 	Experiment_Ui_moc.cc
tgtsrcs_configdb_gui += Transaction_Ui.cc 	Transaction_Ui_moc.cc
tgtsrcs_configdb_gui += Info_Ui.cc 		Info_Ui_moc.cc
tgtsrcs_configdb_gui += ListUi.cc 		ListUi_moc.cc
tgtsrcs_configdb_gui += DetInfoDialog_Ui.cc 	DetInfoDialog_Ui_moc.cc
tgtincs_configdb_gui := $(qtincdir)
tgtlibs_configdb_gui := $(datalibs)
tgtlibs_configdb_gui += qt/QtGui qt/QtCore
tgtlibs_configdb_gui += pdsapp/configdb
tgtslib_configdb_gui := $(USRLIBDIR)/rt

tgtsrcs_configdb_list := configdb_list.cc
tgtsrcs_configdb_list += ListUi.cc ListUi_moc.cc
tgtincs_configdb_list := $(qtincdir)
tgtlibs_configdb_list := $(datalibs)
tgtlibs_configdb_list += qt/QtGui qt/QtCore
tgtlibs_configdb_list += pdsapp/configdb
tgtslib_configdb_list := $(USRLIBDIR)/rt

tgtsrcs_create_scan := create_scan_config.cc
tgtincs_create_scan := $(qtincdir)
tgtlibs_create_scan := $(datalibs)
tgtlibs_create_scan += qt/QtGui qt/QtCore
tgtlibs_create_scan += pdsapp/configdb
tgtslib_create_scan := $(USRLIBDIR)/rt
