#include <stdio.h>

#include "pdsapp/config/SerializerDictionary.hh"

#include "pdsapp/config/Serializer.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/AliasConfig.hh"
#include "pdsapp/config/EvrIOConfig.hh"
#include "pdsapp/config/EvrIOConfig_V1.hh"
#include "pdsapp/config/EvsConfig.hh"
#include "pdsapp/config/EvrConfigP.hh"
#include "pdsapp/config/EvrConfigP_V6.hh"
#include "pdsapp/config/EvrConfig_V5.hh"
#include "pdsapp/config/EvrConfig_V4.hh"
#include "pdsapp/config/EvrConfig_V3.hh"
#include "pdsapp/config/EvrConfig_V2.hh"
#include "pdsapp/config/EvrConfig_V1.hh"
#include "pdsapp/config/AcqConfig.hh"
#include "pdsapp/config/AcqTdcConfig.hh"
#include "pdsapp/config/IpimbConfig_V1.hh"
#include "pdsapp/config/IpimbConfig.hh"
#include "pdsapp/config/IpmFexConfig_V1.hh"
#include "pdsapp/config/IpmFexConfig.hh"
#include "pdsapp/config/DiodeFexConfig_V1.hh"
#include "pdsapp/config/DiodeFexConfig.hh"
#include "pdsapp/config/PimImageConfig.hh"
#include "pdsapp/config/EncoderConfig.hh"
#include "pdsapp/config/EncoderConfig_V1.hh"
#include "pdsapp/config/UsdUsbConfig.hh"
#include "pdsapp/config/Opal1kConfig.hh"
#include "pdsapp/config/QuartzConfig.hh"
#include "pdsapp/config/QuartzConfig_V1.hh"
#include "pdsapp/config/OrcaConfig.hh"
#include "pdsapp/config/FccdConfig.hh"
#include "pdsapp/config/TM6740Config.hh"
#include "pdsapp/config/TM6740ConfigV1.hh"
#include "pdsapp/config/pnCCDConfig.hh"
#include "pdsapp/config/princetonConfig.hh"
#include "pdsapp/config/FrameFexConfig.hh"
#include "pdsapp/config/ControlConfig_V1.hh"
#include "pdsapp/config/ControlConfig.hh"
#include "pdsapp/config/CspadConfig_V1.hh"
#include "pdsapp/config/CspadConfig_V2.hh"
#include "pdsapp/config/CspadConfig_V3.hh"
#include "pdsapp/config/CspadConfig_V4.hh"
#include "pdsapp/config/CspadConfig.hh"
#include "pdsapp/config/ImpConfig.hh"
#include "pdsapp/config/Gsc16aiConfig.hh"
#include "pdsapp/config/TimepixConfig.hh"
#include "pdsapp/config/TimepixConfig_V2.hh"
#include "pdsapp/config/RayonixConfig.hh"
#include "pdsapp/config/RayonixConfig_V1.hh"
#include "pdsapp/config/Cspad2x2Config_V1.hh"
#include "pdsapp/config/Cspad2x2Config.hh"
#include "pdsapp/config/EpixSamplerConfig.hh"
#include "pdsapp/config/EpixConfig.hh"
#include "pdsapp/config/Epix10kConfig.hh"
#include "pdsapp/config/Epix100aConfig.hh"
#include "pdsapp/config/GenericPgpConfig.hh"
#include "pdsapp/config/OceanOpticsConfig.hh"
#include "pdsapp/config/FliConfig.hh"
#include "pdsapp/config/AndorConfig.hh"
#include "pdsapp/config/PimaxConfig.hh"
#include "pdsapp/config/TimeToolConfig.hh"
#include "pdsapp/config/TimeToolConfig_V1.hh"

#include "pds/config/AliasConfigType.hh"
#include "pds/config/EvsConfigType.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/EvrIOConfigType.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/config/EncoderConfigType.hh"
#include "pds/config/UsdUsbConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pds/config/OrcaConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/PrincetonConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/ControlConfigType.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/config/Gsc16aiConfigType.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/OceanOpticsConfigType.hh"
#include "pds/config/FliConfigType.hh"
#include "pds/config/AndorConfigType.hh"
#include "pds/config/PimaxConfigType.hh"
#include "pds/config/TimeToolConfigType.hh"
#include "pds/config/PdsDefs.hh"

#include "pdsdata/psddl/lusi.ddl.h"

#ifdef BUILD_EXTRA
#include "pdsapp/config/XampsConfig.hh"
#include "pdsapp/config/FexampConfig.hh"
#include "pdsapp/config/PhasicsConfig.hh"
#include "pds/config/XampsConfigType.hh"
#include "pds/config/FexampConfigType.hh"
#include "pds/config/PhasicsConfigType.hh"
#endif

using namespace Pds_ConfigDb;

SerializerDictionary::SerializerDictionary()
{
}

SerializerDictionary::~SerializerDictionary()
{
}

Serializer* SerializerDictionary::lookup(const Pds::TypeId& type)
{
#define enroll(_type, v) { if (type.value()==_type.value()) return v; }
  enroll(_encoderConfigType     ,new EncoderConfig);
  enroll(_usdusbConfigType      ,new UsdUsbConfig);
  enroll(_acqConfigType         ,new AcqConfig);
  enroll(_acqTdcConfigType      ,new AcqTdcConfig);
  enroll(_evsConfigType         ,new EvsConfig);
  enroll(_evrConfigType         ,new EvrConfigP);
  enroll(_evrIOConfigType       ,new EvrIOConfig);
  enroll(_opal1kConfigType      ,new Opal1kConfig);
  enroll(_quartzConfigType      ,new QuartzConfig);
  enroll(_orcaConfigType        ,new OrcaConfig);
  enroll(_fccdConfigType        ,new FccdConfig);
  enroll(_tm6740ConfigType      ,new TM6740Config);  
  enroll(_pnCCDConfigType       ,new pnCCDConfig);
  enroll(_frameFexConfigType    ,new FrameFexConfig);
  enroll(_controlConfigType     ,new ControlConfig);
  enroll(_princetonConfigType   ,new princetonConfig);  
  enroll(_ipimbConfigType       ,new IpimbConfig);  
  enroll(_ipmFexConfigType      ,new IpmFexConfig);
  enroll(_diodeFexConfigType    ,new DiodeFexConfig);  
  enroll(_pimImageConfigType    ,new PimImageConfig);  
  enroll(_CsPadConfigType       ,new CspadConfig);
#ifdef BUILD_EXTRA
  enroll(_XampsConfigType       ,new XampsConfig);
  enroll(_FexampConfigType      ,new FexampConfig);
  enroll(_PhasicsConfigType     ,new PhasicsConfig);
#endif
  enroll(_ImpConfigType         ,new ImpConfig);
  enroll(_gsc16aiConfigType     ,new Gsc16aiConfig);
  enroll(_timepixConfigType     ,new TimepixConfig);
  enroll(_rayonixConfigType     ,new RayonixConfig);
  enroll(_CsPad2x2ConfigType    ,new Cspad2x2Config);
  enroll(_epixSamplerConfigType ,new EpixSamplerConfig);
  enroll(_epixConfigType        ,new EpixConfig);
  enroll(_epix10kConfigType     ,new Epix10kConfig);
  enroll(_epix100aConfigType     ,new Epix100aConfig);
  enroll(_genericPgpConfigType  ,new GenericPgpConfig);
  enroll(_oceanOpticsConfigType ,new OceanOpticsConfig);  
  enroll(_fliConfigType         ,new FliConfig);  
  enroll(_andorConfigType       ,new AndorConfig);
  enroll(_pimaxConfigType       ,new PimaxConfig);
  enroll(_timetoolConfigType    ,new TimeToolConfig);
  //  retired
  enroll(Pds::TypeId(Pds::TypeId::Id_Cspad2x2Config,1),new Cspad2x2Config_V1);
  enroll(Pds::TypeId(Pds::TypeId::Id_CspadConfig,4) , new CspadConfig_V4);
  enroll(Pds::TypeId(Pds::TypeId::Id_CspadConfig,3) , new CspadConfig_V3);
  enroll(Pds::TypeId(Pds::TypeId::Id_CspadConfig,2) , new CspadConfig_V2);
  enroll(Pds::TypeId(Pds::TypeId::Id_CspadConfig,1) , new CspadConfig_V1);  
  enroll(Pds::TypeId(Pds::TypeId::Id_EncoderConfig,1),new EncoderConfig_V1);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,6)   , new EvrConfig_V6::EvrConfigP);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,5)   , new EvrConfig_V5::EvrConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,4)   , new EvrConfig_V4::EvrConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,3)   , new EvrConfig_V3::EvrConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,2)   , new EvrConfig_V2::EvrConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,1)   , new EvrConfig_V1::EvrConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrIOConfig,1) , new EvrIOConfig_V1::EvrIOConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_TM6740Config,1), new TM6740ConfigV1);  
  enroll(Pds::TypeId(Pds::TypeId::Id_IpimbConfig,1) , new IpimbConfig_V1);  
  enroll(Pds::TypeId(Pds::TypeId::Id_IpmFexConfig,1), new IpmFexConfig_V1);
  enroll(Pds::TypeId(Pds::TypeId::Id_DiodeFexConfig,1), new DiodeFexConfig_V1);  
  enroll(Pds::TypeId(Pds::TypeId::Id_ControlConfig,1),new ControlConfig_V1::ControlConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_TimepixConfig,2),new TimepixConfig_V2);
  enroll(Pds::TypeId(Pds::TypeId::Id_TimeToolConfig,1),new V1::TimeToolConfig);
  enroll(Pds::TypeId(Pds::TypeId::Id_QuartzConfig,1),new V1::QuartzConfig);
//  enroll(Pds::TypeId(Pds::TypeId::Id_RayonixConfig,1), new RayonixConfig_V1);

  if (Parameter::readFromData())
    enroll(_aliasConfigType, new AliasConfig);

#undef enroll

  printf("Unable to find serializer for %s_v%d\n",
	 Pds::TypeId::name(type.id()),type.version());

  return 0;
}
