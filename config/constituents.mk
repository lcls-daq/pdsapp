ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
qtincdir  := qt/include_64
else
qtincdir  := qt/include
endif

libnames       := configdb configdbg
libsrcs_configdb := Path.cc
libsrcs_configdb += Table.cc
libsrcs_configdb += Device.cc
libsrcs_configdb += DeviceEntry.cc
libsrcs_configdb += Experiment.cc
libsrcs_configdb += GlobalCfg.cc
libsrcs_configdb += PdsDefs.cc
libsrcs_configdb += EventcodeTiming.cc

libsrcs_configdbg := ControlScan.cc ControlScan_moc.cc
libsrcs_configdbg += PvScan.cc PvScan_moc.cc
libsrcs_configdbg += EvrScan.cc EvrScan_moc.cc
libsrcs_configdbg += Reconfig_Ui.cc Reconfig_Ui_moc.cc
libsrcs_configdbg += Dialog.cc     Dialog_moc.cc 
libsrcs_configdbg += SubDialog.cc    SubDialog_moc.cc 
libsrcs_configdbg += Validators.cc     Validators_moc.cc 
libsrcs_configdbg += ParameterSet.cc   ParameterSet_moc.cc
libsrcs_configdbg += QrLabel.cc   QrLabel_moc.cc
libsrcs_configdbg += SerializerDictionary.cc
libsrcs_configdbg += ExpertDictionary.cc
libsrcs_configdbg += Serializer.cc
libsrcs_configdbg += Opal1kConfig.cc
libsrcs_configdbg += FccdConfig.cc
libsrcs_configdbg += pnCCDConfig.cc
libsrcs_configdbg += princetonConfig.cc
libsrcs_configdbg += TM6740Config.cc
libsrcs_configdbg += TM6740ConfigV1.cc
libsrcs_configdbg += FrameFexConfig.cc
libsrcs_configdbg += EvrOutputMap.cc EvrPulseConfig.cc EvrPulseConfig_V1.cc EvrEventCodeV3.cc EvrEventCode.cc
libsrcs_configdbg += EvrIOChannel.cc EvrIOChannel_moc.cc
libsrcs_configdbg += EvrIOConfig.cc EvrIOConfig_moc.cc
libsrcs_configdbg += PolarityButton.cc
libsrcs_configdbg += EvrPulseTable_V4.cc EvrPulseTable_V4_moc.cc
libsrcs_configdbg += EvrPulseTable.cc EvrPulseTable_moc.cc
libsrcs_configdbg += EvrEventDesc.cc EvrEventDesc_moc.cc
libsrcs_configdbg += EvrSeqEventDesc.cc
libsrcs_configdbg += EvrGlbEventDesc.cc
libsrcs_configdbg += EvrEventCodeTable.cc EvrEventCodeTable_moc.cc
libsrcs_configdbg += EvrConfig.cc EvrConfigP.cc EvrConfig_V4.cc EvrConfig_V3.cc EvrConfig_V2.cc EvrConfig_V1.cc
libsrcs_configdbg += SequencerConfig.cc SequencerConfig_moc.cc
libsrcs_configdbg += CspadConfigTable_V1.cc 
libsrcs_configdbg += CspadConfig_V1.cc
libsrcs_configdbg += CspadConfigTable_V2.cc 
libsrcs_configdbg += CspadConfig_V2.cc CspadConfigTable_V2_moc.cc
libsrcs_configdbg += CspadSector.cc
libsrcs_configdbg += CspadGainMap.cc CspadGainMap_moc.cc
libsrcs_configdbg += CspadConfigTable.cc CspadConfigTable_moc.cc
libsrcs_configdbg += CspadConfig.cc
libsrcs_configdbg += XampsConfig.cc XampsConfig_moc.cc
libsrcs_configdbg += XampsCopyChannelDialog.cc XampsCopyChannelDialog_moc.cc
libsrcs_configdbg += XampsCopyAsicDialog.cc XampsCopyAsicDialog_moc.cc
libsrcs_configdbg += FexampConfig.cc FexampConfig_moc.cc
libsrcs_configdbg += FexampCopyChannelDialog.cc FexampCopyChannelDialog_moc.cc
libsrcs_configdbg += FexampCopyAsicDialog.cc FexampCopyAsicDialog_moc.cc
libsrcs_configdbg += PhasicsConfig.cc PhasicsConfig_moc.cc
libsrcs_configdbg += ControlConfig.cc
libsrcs_configdbg += IpimbConfig.cc IpimbConfig_V1.cc
libsrcs_configdbg += DiodeFexItem.cc
libsrcs_configdbg += IpmFexTable.cc
libsrcs_configdbg += IpmFexConfig.cc IpmFexConfig_V1.cc
libsrcs_configdbg += DiodeFexTable.cc
libsrcs_configdbg += DiodeFexConfig.cc DiodeFexConfig_V1.cc
libsrcs_configdbg += PimImageConfig.cc
libsrcs_configdbg += EncoderConfig.cc EncoderConfig_V1.cc
libsrcs_configdbg += Gsc16aiConfig.cc
libsrcs_configdbg += TimepixConfig.cc
libsrcs_configdbg += AcqChannelMask.cc AcqChannelMask_moc.cc
libsrcs_configdbg += AcqConfig.cc
libsrcs_configdbg += AcqTdcConfig.cc
libsrcs_configdbg += Parameters.cc
libsrcs_configdbg += BitCount.cc
libsrcs_configdbg += templates.cc
libincs_configdbg := $(qtincdir)

tgtnames       := configdb
tgtnames       += configdb_gui
tgtnames       += configdb_list
#tgtnames       += create_scan

# executable python modules: configdb_gui.py

datalibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata pdsdata/encoderdata pdsdata/fccddata pdsdata/lusidata pdsdata/cspaddata pdsdata/xampsdata pdsdata/fexampdata pdsdata/gsc16aidata
datalibs += pdsdata/timepixdata
datalibs += pdsdata/phasicsdata

tgtsrcs_configdb := configdb.cc
#tgtincs_configdb := $(qtincdir)
tgtlibs_configdb := $(datalibs)
tgtlibs_configdb += pdsapp/configdb
#tgtlibs_configdb += pdsapp/configdbg
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
tgtlibs_configdb_gui += pdsapp/configdbg
tgtslib_configdb_gui := $(USRLIBDIR)/rt

tgtsrcs_configdb_list := configdb_list.cc
tgtsrcs_configdb_list += ListUi.cc ListUi_moc.cc
tgtincs_configdb_list := $(qtincdir)
tgtlibs_configdb_list := $(datalibs)
tgtlibs_configdb_list += qt/QtGui qt/QtCore
tgtlibs_configdb_list += pdsapp/configdb
tgtlibs_configdb_list += pdsapp/configdbg
tgtslib_configdb_list := $(USRLIBDIR)/rt

tgtsrcs_create_scan := create_scan_config.cc
tgtincs_create_scan := $(qtincdir)
tgtlibs_create_scan := $(datalibs)
tgtlibs_create_scan += qt/QtGui qt/QtCore
tgtlibs_create_scan += pdsapp/configdb
tgtlibs_create_scan += pdsapp/configdbg
tgtslib_create_scan := $(USRLIBDIR)/rt
