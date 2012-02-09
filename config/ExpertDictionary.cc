#include "pdsapp/config/ExpertDictionary.hh"

#include "pdsapp/config/EvrConfig.hh"
#include "pdsapp/config/PhasicsConfig.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

ExpertDictionary::ExpertDictionary()
{
  enroll(_evrConfigType   , new EvrConfig);
  enroll(_PhasicsConfigType   ,new PhasicsExpertConfig);
}

ExpertDictionary::~ExpertDictionary()
{
}
