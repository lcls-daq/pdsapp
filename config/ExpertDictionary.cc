#include "pdsapp/config/ExpertDictionary.hh"

#include "pdsapp/config/EvrConfig.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

ExpertDictionary::ExpertDictionary()
{
  enroll(_evrConfigType   , new EvrConfig);
}

ExpertDictionary::~ExpertDictionary()
{
}
