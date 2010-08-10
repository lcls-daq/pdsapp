#include "pdsapp/config/SerializerDictionary.hh"

#include "pdsapp/config/Serializer.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/EvrIOConfig.hh"
#include "pdsapp/config/EvrConfigP.hh"
#include "pdsapp/config/EvrConfig_V3.hh"
#include "pdsapp/config/EvrConfig_V2.hh"
#include "pdsapp/config/EvrConfig_V1.hh"
#include "pdsapp/config/AcqConfig.hh"
#include "pdsapp/config/IpimbConfig.hh"
#include "pdsapp/config/IpmFexConfig.hh"
#include "pdsapp/config/DiodeFexConfig.hh"
#include "pdsapp/config/PimImageConfig.hh"
#include "pdsapp/config/EncoderConfig.hh"
#include "pdsapp/config/Opal1kConfig.hh"
#include "pdsapp/config/FccdConfig.hh"
#include "pdsapp/config/TM6740Config.hh"
#include "pdsapp/config/TM6740ConfigV1.hh"
#include "pdsapp/config/pnCCDConfig.hh"
#include "pdsapp/config/princetonConfig.hh"
#include "pdsapp/config/FrameFexConfig.hh"
#include "pdsapp/config/ControlConfig.hh"

#include "pds/config/EvrConfigType.hh"
#include "pds/config/EvrIOConfigType.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/config/EncoderConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/PrincetonConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/ControlConfigType.hh"

using namespace Pds_ConfigDb;

SerializerDEntry::SerializerDEntry(const Pds::TypeId& t,
           Serializer* s) :
  type(t), serializer(s) {}

SerializerDEntry::~SerializerDEntry()
{
}

bool SerializerDEntry::operator==(const SerializerDEntry& s) const
{
  return type.value()==s.type.value();
}


SerializerDictionary::SerializerDictionary()
{
  enroll(_encoderConfigType   ,new EncoderConfig);
  enroll(_acqConfigType       ,new AcqConfig);
  enroll(_evrConfigType       ,new EvrConfigP);
  enroll(_evrIOConfigType     ,new EvrIOConfig);
  enroll(_opal1kConfigType    ,new Opal1kConfig);
  enroll(_fccdConfigType      ,new FccdConfig);
  enroll(_tm6740ConfigType    ,new TM6740Config);  
  enroll(_pnCCDConfigType     ,new pnCCDConfig);
  enroll(_frameFexConfigType  ,new FrameFexConfig);
  enroll(_controlConfigType   ,new ControlConfig);
  enroll(_princetonConfigType ,new princetonConfig);  
  enroll(_ipimbConfigType     ,new IpimbConfig);  
  enroll(_ipmFexConfigType    ,new IpmFexConfig);  
  enroll(_diodeFexConfigType  ,new DiodeFexConfig);  
  enroll(_pimImageConfigType  ,new PimImageConfig);  
  //  retired
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,3), new EvrConfig_V3);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,2), new EvrConfig_V2);
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,1), new EvrConfig_V1);
  enroll(Pds::TypeId(Pds::TypeId::Id_TM6740Config,1), new TM6740ConfigV1);  
}

SerializerDictionary::~SerializerDictionary()
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    delete iter->serializer;
}

void SerializerDictionary::enroll(const Pds::TypeId& type,
				  Serializer* s)
{
  SerializerDEntry entry(type,s);
  _list.remove(entry);
  _list.push_back(entry);
}

Serializer* SerializerDictionary::lookup(const Pds::TypeId& type)
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    if (iter->type.value()==type.value())
      return iter->serializer;
  return 0;
}
