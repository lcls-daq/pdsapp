#include "L3TestModule.hh"

//#define DBUG

using namespace Pds;

L3TestModule::L3TestModule() : _evr(0) {}

std::string L3TestModule::name() const
{
  return std::string("pdsapp/tools/L3TestModule");
}

std::string L3TestModule::configuration() const
{
  return std::string("Accepts eventcode 42");
}

void L3TestModule::event(const Pds::DetInfo& src,
                         const Pds::TypeId&  type,
                         void*               payload)
{
#ifdef DBUG
  printf("L3TModule::event %08x.%08x %08x\n",
         src.log(),src.phy(),type.value());
#endif
  if (type.id()==TypeId::Id_EvrData && type.version()==3)
    _evr = reinterpret_cast<const EvrData::DataV3*>(payload);
}

bool L3TestModule::accept()
{
  if (_evr) {
    ndarray<const EvrData::FIFOEvent,1> a = _evr->fifoEvents();
    _evr = 0;
    for(const EvrData::FIFOEvent* it=a.begin(); it!=a.end(); it++)
      if ((*it).eventCode()==42) {
#ifdef DBUG
        printf("L3TModule::accept ec 42 ts %08x.%08x\n", 
               (*it).timestampHigh(),
               (*it).timestampLow ());
#endif
        return true;
      }
  }
#ifdef DBUG
  printf("L3TModule::accept ec 42 missing\n");
#endif
  return false;
}

extern "C" L3FilterModule* create() { return new L3TestModule; }

