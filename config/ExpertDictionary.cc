#include "pdsapp/config/ExpertDictionary.hh"

#include "pdsapp/config/EvrConfig.hh"
#include "pdsapp/config/PhasicsConfig.hh"
#include "pdsapp/config/TimepixConfig.hh"

#include "pds/config/EvrConfigType.hh"
#include "pds/config/TimepixConfigType.hh"

using namespace Pds_ConfigDb;

ExpertDictionary::ExpertDictionary()
{
  enroll(_evrConfigType   , new EvrConfig);
  enroll(_PhasicsConfigType   ,new PhasicsExpertConfig);
  enroll(_timepixConfigType   ,new TimepixExpertConfig);
}

ExpertDictionary::~ExpertDictionary()
{
}
