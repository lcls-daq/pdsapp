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
  return std::string("Accepts fiducial&0xf==0");
}

void L3TestModule::pre_event() { _evr=-1U; }

void L3TestModule::event(const Pds::DetInfo& src,
                         const Pds::TypeId&  type,
                         void*               payload)
{
#ifdef DBUG
  printf("L3TModule::event %08x.%08x %08x\n",
         src.log(),src.phy(),type.value());
#endif
  if (src.device()==DetInfo::Evr && type.id()==TypeId::Any)
    _evr = *reinterpret_cast<unsigned*>(payload);
}

bool L3TestModule::complete()
{ return _evr!=-1U; }

bool L3TestModule::accept()
{
  return (_evr&0xf)==0;
}

extern "C" L3FilterModule* create() { return new L3TestModule; }

