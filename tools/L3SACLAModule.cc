#include "L3SACLAModule.hh"

//#define DBUG

using namespace Pds;

L3SACLAModule::L3SACLAModule() : _din(0), _dataSeen(false) {}

std::string L3SACLAModule::name() const
{
  return std::string("pdsapp/tools/L3SACLAModule");
}

std::string L3SACLAModule::configuration() const
{
  return std::string("L3 filter accepts UsdUsb::DataV1");
}

void L3SACLAModule::pre_event() {
  _dataSeen = false;
}

void L3SACLAModule::event(const Pds::DetInfo& src,
                         const Pds::TypeId&  type,
                         void*               payload)
{
#ifdef DBUG
  printf("L3TModule::event %08x.%08x %08x\n",
         src.log(),src.phy(),type.value());
#endif
  if (type.id()==TypeId::Id_UsdUsbData && type.version()==1) {
    const UsdUsb::DataV1* dat = reinterpret_cast<const UsdUsb::DataV1*>(payload);
    _din = dat->digital_in();
    _dataSeen = true;
  }
}

bool L3SACLAModule::complete()
{ return _dataSeen; }

bool L3SACLAModule::accept()
{
  if (_dataSeen && ((_din&1)==0))  {
#ifdef DBUG
        printf("L3TModule::accept _din %x ts %08x.%08x\n",
               _din,
               (*it).timestampHigh(),
               (*it).timestampLow ());
#endif
        return true;
      }
#ifdef DBUG
  printf("L3TModule::accept _din %x\n", _din);
#endif
  return false;
}

extern "C" L3FilterModule* create() { return new L3SACLAModule; }

