#include "pdsapp/config/PdsDefs.hh"

using namespace Pds_ConfigDb;

static string _type_ids[Pds::TypeId::NumberOf+1];
static string _detector[Pds::DetInfo::NumDetector+1];
static string _device  [Pds::DetInfo::NumDevice+1];
static string _eolist;

#define TYPE_ID(id)  { _type_ids[Pds::TypeId::id] = string(#id); }
#define DETECTOR(id) { _detector[Pds::DetInfo::id] = string(#id); }
#define DEVICE(id)   { _device  [Pds::DetInfo::id] = string(#id); }

void PdsDefs::initialize()
{
  TYPE_ID(Any);
  TYPE_ID(Id_Xtc);
  TYPE_ID(Id_Frame);
  TYPE_ID(Id_AcqWaveform);
  TYPE_ID(Id_AcqConfig);
  TYPE_ID(Id_TwoDGaussian);
  TYPE_ID(Id_Opal1kConfig);
  TYPE_ID(Id_FrameFexConfig);
  TYPE_ID(Id_EvrConfig);
  _type_ids[Pds::TypeId::NumberOf] = _eolist;
  
  DETECTOR(NoDetector);
  DETECTOR(AmoIms);
  DETECTOR(AmoPem);
  DETECTOR(AmoETof);
  DETECTOR(AmoITof);
  DETECTOR(AmoMbs);
  DETECTOR(AmoIis);
  DETECTOR(AmoXes);
  _detector[Pds::DetInfo::NumDetector] = _eolist;

  DEVICE(NoDevice);
  DEVICE(Evr);
  DEVICE(Acqiris);
  DEVICE(Opal1000);
  DEVICE(NumDevice);
  _device[Pds::DetInfo::NumDevice] = _eolist;
}

const string& PdsDefs::type_id(unsigned i)
{
  return _type_ids[i];
}

unsigned PdsDefs::type_index(const string& t)
{
  for(unsigned i=0; i<Pds::TypeId::NumberOf; i++)
    if (t == _type_ids[i])
      return i;
  return Pds::TypeId::NumberOf;
}

const string& PdsDefs::detector(unsigned i)
{
  return _detector[i];
}

const string& PdsDefs::device  (unsigned i)
{
  return _device[i];
}
