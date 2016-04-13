#include "pdsapp/config/ExpertDictionary.hh"

#include "pdsapp/config/EvrConfig.hh"
#include "pdsapp/config/TimepixConfig.hh"
#include "pdsapp/config/RayonixConfig.hh"
#include "pdsapp/config/Epix100aConfig.hh"

#ifdef BUILD_EXTRA
#include "pdsapp/config/PhasicsConfig.hh"
#endif

#include "pds/config/EvrConfigType.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/config/EpixConfigType.hh"

using namespace Pds_ConfigDb;

ExpertDictionary::ExpertDictionary()
{
}

ExpertDictionary::~ExpertDictionary()
{
}

Serializer* ExpertDictionary::lookup(const Pds::TypeId& type)
{
#define enroll(_type, v) { if (type.value()==_type.value()) return v; }
  enroll(_evrConfigType   , new EvrConfig);
  enroll(_timepixConfigType   ,new TimepixExpertConfig);
  enroll(_rayonixConfigType,   new RayonixExpertConfig);
  enroll(_epix100aConfigType    ,new Epix100aConfig);
#ifdef BUILD_EXTRA
  enroll(_PhasicsConfigType   ,new PhasicsExpertConfig);
#endif
#undef enroll
  return SerializerDictionary::lookup(type);
}
