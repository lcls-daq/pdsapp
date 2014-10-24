libnames       := configdb
ifneq ($(findstring x86_64,$(tgt_arch)),)
libnames       += configdbg
endif

libsrcs_configdb := Table.cc
libsrcs_configdb += Device.cc
libsrcs_configdb += Experiment.cc
libsrcs_configdb += EventcodeTiming.cc
libsrcs_configdb += GlobalCfg.cc
libincs_configdb := pdsdata/include ndarray/include boost/include  

libsrcs_configdbg := ControlScan.cc ControlScan_moc.cc
libsrcs_configdbg += PvScan.cc PvScan_moc.cc
libsrcs_configdbg += EvrScan.cc EvrScan_moc.cc
libsrcs_configdbg += Reconfig_Ui.cc Reconfig_Ui_moc.cc
libsrcs_configdbg += Restore_Ui.cc Restore_Ui_moc.cc
libsrcs_configdbg += Dialog.cc     Dialog_moc.cc 
libsrcs_configdbg += SubDialog.cc    SubDialog_moc.cc 
libsrcs_configdbg += Validators.cc     Validators_moc.cc 
libsrcs_configdbg += ParameterSet.cc   ParameterSet_moc.cc
libsrcs_configdbg += PolyDialog.cc   PolyDialog_moc.cc
libsrcs_configdbg += QrLabel.cc   QrLabel_moc.cc
libsrcs_configdbg += EventLogic.cc EventLogic_moc.cc
libsrcs_configdbg += SerializerDictionary.cc
libsrcs_configdbg += ExpertDictionary.cc
libsrcs_configdbg += Serializer.cc
libsrcs_configdbg += AliasConfig.cc
libsrcs_configdbg += Opal1kConfig.cc
libsrcs_configdbg += QuartzConfig.cc
libsrcs_configdbg += OrcaConfig.cc
libsrcs_configdbg += FccdConfig.cc
libsrcs_configdbg += pnCCDConfig.cc
libsrcs_configdbg += princetonConfig.cc
libsrcs_configdbg += FliConfig.cc
libsrcs_configdbg += AndorConfig.cc
libsrcs_configdbg += PimaxConfig.cc
libsrcs_configdbg += TM6740Config.cc
libsrcs_configdbg += TM6740ConfigV1.cc
libsrcs_configdbg += FrameFexConfig.cc
#libsrcs_configdbg += ProjectionConfig.cc ProjectionConfigQ.cc ProjectionConfigQ_moc.cc
libsrcs_configdbg += EvrOutputMap.cc EvrOutputMap_V1.cc
libsrcs_configdbg += EvrPulseConfig.cc EvrPulseConfig_V1.cc EvrEventCodeV3.cc EvrEventCode.cc
libsrcs_configdbg += EvrIOChannel.cc EvrIOChannel_moc.cc
libsrcs_configdbg += EvrIOChannel_V1.cc EvrIOChannel_V1_moc.cc
libsrcs_configdbg += EvrIOConfig.cc EvrIOConfig_moc.cc
libsrcs_configdbg += EvrIOConfig_V1.cc EvrIOConfig_V1_moc.cc
libsrcs_configdbg += PolarityButton.cc
libsrcs_configdbg += EvrPulseTable_V6.cc EvrPulseTable_V6_moc.cc
libsrcs_configdbg += EvrPulseTable_V5.cc EvrPulseTable_V5_moc.cc
libsrcs_configdbg += EvrPulseTable_V4.cc EvrPulseTable_V4_moc.cc
libsrcs_configdbg += EvrPulseTable.cc EvrPulseTable_moc.cc
libsrcs_configdbg += EvrEventDesc.cc EvrEventDesc_moc.cc
libsrcs_configdbg += EvrEventDefault.cc
libsrcs_configdbg += EvrSeqEventDesc.cc
libsrcs_configdbg += EvrGlbEventDesc.cc
libsrcs_configdbg += EvrEventDesc_V6.cc EvrEventDesc_V6_moc.cc
libsrcs_configdbg += EvrSeqEventDesc_V6.cc
libsrcs_configdbg += EvrGlbEventDesc_V6.cc
libsrcs_configdbg += EvrEventCodeTable.cc EvrEventCodeTable_moc.cc
libsrcs_configdbg += EvrEventCodeTable_V6.cc EvrEventCodeTable_V6_moc.cc
libsrcs_configdbg += EvsPulseTable.cc EvsPulseTable_moc.cc
libsrcs_configdbg += EvsConfig.cc
libsrcs_configdbg += EvrConfigType_V6.cc
libsrcs_configdbg += EvrConfig.cc EvrConfigP.cc EvrConfigP_V6.cc EvrConfig_V5.cc EvrConfig_V4.cc EvrConfig_V3.cc EvrConfig_V2.cc EvrConfig_V1.cc
libsrcs_configdbg += SequencerConfig_V6.cc SequencerConfig_V6_moc.cc
libsrcs_configdbg += SequencerConfig.cc SequencerConfig_moc.cc
libsrcs_configdbg += CspadConfigTable_V1.cc 
libsrcs_configdbg += CspadConfig_V1.cc
libsrcs_configdbg += CspadConfigTable_V2.cc 
libsrcs_configdbg += CspadConfig_V2.cc CspadConfigTable_V2_moc.cc
libsrcs_configdbg += CspadConfigTable_V3.cc 
libsrcs_configdbg += CspadConfig_V3.cc CspadConfigTable_V3_moc.cc
libsrcs_configdbg += CspadConfigTable_V4.cc 
libsrcs_configdbg += CspadConfig_V4.cc CspadConfigTable_V4_moc.cc
libsrcs_configdbg += CspadSector.cc
libsrcs_configdbg += CspadGainMap.cc CspadGainMap_moc.cc
libsrcs_configdbg += CspadConfigTable.cc CspadConfigTable_moc.cc
libsrcs_configdbg += CspadConfig.cc
libsrcs_configdbg += Cspad2x2Temp.cc
libsrcs_configdbg += Cspad2x2Config.cc
libsrcs_configdbg += Cspad2x2Config_V1.cc
libsrcs_configdbg += Cspad2x2Sector.cc
libsrcs_configdbg += Cspad2x2GainMap.cc Cspad2x2GainMap_moc.cc
libsrcs_configdbg += Cspad2x2ConfigTable.cc Cspad2x2ConfigTable_moc.cc
libsrcs_configdbg += Cspad2x2ConfigTable_V1.cc Cspad2x2ConfigTable_V1_moc.cc
libsrcs_configdbg += ImpConfig.cc
libsrcs_configdbg += PVControl.cc PVMonitor.cc
libsrcs_configdbg += ControlConfig_V1.cc
libsrcs_configdbg += ControlConfig.cc
libsrcs_configdbg += ControlConfigType_V1.cc
libsrcs_configdbg += IpimbConfig.cc IpimbConfig_V1.cc
libsrcs_configdbg += DiodeFexItem.cc
libsrcs_configdbg += IpmFexTable.cc
libsrcs_configdbg += IpmFexConfig.cc IpmFexConfig_V1.cc
libsrcs_configdbg += DiodeFexTable.cc
libsrcs_configdbg += DiodeFexConfig.cc DiodeFexConfig_V1.cc
libsrcs_configdbg += PimImageConfig.cc
libsrcs_configdbg += EncoderConfig.cc EncoderConfig_V1.cc
libsrcs_configdbg += UsdUsbConfig.cc
libsrcs_configdbg += Gsc16aiConfig.cc
libsrcs_configdbg += TimepixConfig.cc TimepixConfig_V2.cc
libsrcs_configdbg += RayonixConfig.cc RayonixConfig_V1.cc
libsrcs_configdbg += EpixSamplerConfig.cc
libsrcs_configdbg += EpixConfig.cc EpixConfigP.cc EpixCopyAsicDialog.cc EpixCopyAsicDialog_moc.cc
libsrcs_configdbg += Epix10kConfig.cc Epix10kConfigP.cc Epix10kCopyAsicDialog.cc Epix10kCopyAsicDialog_moc.cc
libsrcs_configdbg += SequenceFactory.cc
libsrcs_configdbg += GenericEpixConfig.cc GenericEpix10kConfig.cc
libsrcs_configdbg += GenericPgpConfig.cc GenericPgpConfig_moc.cc
libsrcs_configdbg += TimeToolConfig.cc
libsrcs_configdbg += Epix100aConfig.cc Epix100aCopyAsicDialog.cc Epix100aCopyAsicDialog_moc.cc
libsrcs_configdbg += Epix100aPixelMap.cc Epix100aPixelMap_moc.cc
libsrcs_configdbg += Epix100aCalibMap.cc Epix100aCalibMap_moc.cc
libsrcs_configdbg += OceanOpticsConfig.cc
libsrcs_configdbg += AcqChannelMask.cc AcqChannelMask_moc.cc
libsrcs_configdbg += AcqConfig.cc
libsrcs_configdbg += AcqTdcConfig.cc
libsrcs_configdbg += QtConcealer.cc QtConcealer_moc.cc
libsrcs_configdbg += Parameters.cc
libsrcs_configdbg += BitCount.cc
libsrcs_configdbg += templates.cc
libincs_configdbg := $(qtincdir) pdsdata/include ndarray/include boost/include   
libincs_configdbg += configdb/include

ifeq ($(build_extra),$(true))
  DEFINES += -DBUILD_EXTRA
  libsrcs_configdbg += XampsConfig.cc XampsConfig_moc.cc
  libsrcs_configdbg += XampsCopyChannelDialog.cc XampsCopyChannelDialog_moc.cc
  libsrcs_configdbg += XampsCopyAsicDialog.cc XampsCopyAsicDialog_moc.cc
  libsrcs_configdbg += FexampConfig.cc FexampConfig_moc.cc
  libsrcs_configdbg += FexampCopyChannelDialog.cc FexampCopyChannelDialog_moc.cc
  libsrcs_configdbg += FexampCopyAsicDialog.cc FexampCopyAsicDialog_moc.cc
  libsrcs_configdbg += PhasicsConfig.cc
endif

tgtnames       := configdb_cmd
ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames       += configdb_gui
tgtnames       += configdb_list
tgtnames       += configdb_readxtc
#tgtnames       += create_scan
tgtnames       += dbtest
endif

# executable python modules: configdb_gui.py

datalibs := pdsdata/xtcdata pdsdata/psddl_pdsdata

tgtsrcs_configdb_cmd := configdb.cc
tgtincs_configdb_cmd := pdsdata/include
tgtlibs_configdb_cmd := $(datalibs)
tgtlibs_configdb_cmd += pdsapp/configdb
tgtlibs_configdb_cmd += pds/configdbc pds/confignfs pds/configsql
tgtlibs_configdb_cmd += pds/config pds/utility pds/collection pds/service pds/vmon pds/mon pds/xtc
tgtlibs_configdb_cmd += offlinedb/mysqlclient
tgtslib_configdb_cmd := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_configdb_gui := configdb_gui.cc
tgtsrcs_configdb_gui += Ui.cc
tgtsrcs_configdb_gui += Devices_Ui.cc     Devices_Ui_moc.cc
tgtsrcs_configdb_gui += Experiment_Ui.cc  Experiment_Ui_moc.cc
tgtsrcs_configdb_gui += Transaction_Ui.cc   Transaction_Ui_moc.cc
tgtsrcs_configdb_gui += Info_Ui.cc    Info_Ui_moc.cc
tgtsrcs_configdb_gui += ListUi.cc     ListUi_moc.cc
tgtsrcs_configdb_gui += DetInfoDialog_Ui.cc   DetInfoDialog_Ui_moc.cc
tgtincs_configdb_gui := $(qtincdir) pdsdata/include configdb/include
tgtlibs_configdb_gui := $(datalibs)
tgtlibs_configdb_gui += $(qtlibdir)
tgtlibs_configdb_gui += pds/configdata
tgtlibs_configdb_gui += pds/configdbc
tgtlibs_configdb_gui += pds/confignfs pds/configsql
tgtlibs_configdb_gui += pdsapp/configdb
tgtlibs_configdb_gui += pdsapp/configdbg
tgtlibs_configdb_gui += offlinedb/mysqlclient
tgtslib_configdb_gui := $(USRLIBDIR)/rt $(qtslibdir) $(USRLIBDIR)/mysql/mysqlclient


tgtsrcs_configdb_list := configdb_list.cc
tgtsrcs_configdb_list += ListUi.cc ListUi_moc.cc
tgtincs_configdb_list := $(qtincdir) pdsdata/include
tgtlibs_configdb_list := $(datalibs)
tgtlibs_configdb_list += $(qtlibdir)
tgtlibs_configdb_list += pds/configdata
tgtlibs_configdb_list += pds/configdbc
tgtlibs_configdb_list += pds/confignfs pds/configsql
tgtlibs_configdb_list += pdsapp/configdb
tgtlibs_configdb_list += pdsapp/configdbg
tgtlibs_configdb_list += offlinedb/mysqlclient
tgtslib_configdb_list := $(USRLIBDIR)/rt $(qtslibdir) $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_create_scan := create_scan_config.cc
tgtincs_create_scan := $(qtincdir) pdsdata/include
tgtlibs_create_scan := $(datalibs)
tgtlibs_create_scan += $(qtlibdir)
tgtlibs_create_scan += pdsapp/configdb
tgtlibs_create_scan += pdsapp/configdbg
tgtslib_create_scan := $(USRLIBDIR)/rt $(qtslibdir)

tgtsrcs_configdb_readxtc := configdb_readxtc.cc
tgtsrcs_configdb_readxtc += Xtc_Ui.cc Xtc_Ui_moc.cc
tgtsrcs_configdb_readxtc += XtcFileServer.cc XtcFileServer_moc.cc
tgtincs_configdb_readxtc := $(qtincdir) pdsdata/include
tgtlibs_configdb_readxtc := $(datalibs)
tgtlibs_configdb_readxtc += $(qtlibdir)
tgtlibs_configdb_readxtc += pds/configdata
tgtlibs_configdb_readxtc += pds/configdbc
tgtlibs_configdb_readxtc += pds/confignfs pds/configsql
tgtlibs_configdb_readxtc += pdsapp/configdb
tgtlibs_configdb_readxtc += pdsapp/configdbg
tgtslib_configdb_readxtc := $(USRLIBDIR)/rt $(qtslibdir) $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_nfs_to_sql := nfs_to_sql.cc
tgtincs_nfs_to_sql := pdsdata/include
tgtlibs_nfs_to_sql := pdsdata/xtcdata
tgtlibs_nfs_to_sql += pds/configdbc
tgtlibs_nfs_to_sql += pds/confignfs pds/configsql
tgtlibs_nfs_to_sql += offlinedb/mysqlclient
tgtslib_nfs_to_sql := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_dbtest := dbtest.cc
tgtincs_dbtest := pdsdata/include
tgtlibs_dbtest += pds/configdata
tgtlibs_dbtest += pds/configdbc
tgtlibs_dbtest += pds/confignfs pds/configsql
tgtlibs_dbtest += pds/service
tgtlibs_dbtest += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtslib_dbtest := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient

