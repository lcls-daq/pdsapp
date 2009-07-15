#include "PVControl.hh"

#include "pdsapp/control/EpicsCA.hh"

#include "pdsdata/control/PVControl.hh"

#include "alarm.h"

namespace Pds {

  class ControlCA : public EpicsCA {
  public:
    ControlCA(PVControl& control,
	      const Pds::ControlData::PVControl& channel) :
      //      EpicsCA(channel.name(), DBR_FLOAT),
      EpicsCA(channel.name()),
      _control(control), _channel(channel) {}
    virtual ~ControlCA() {}
  public:
    void dataCallback(const void* dbr) 
    {
      // read the data, write the data
      //      setVar();
    }
  public:
    const Pds::ControlData::PVControl& channel () const { return _channel; }
  private:
    PVControl& _control;
    Pds::ControlData::PVControl _channel;
  };

};

using namespace Pds;

PVControl::PVControl ()
{
}

PVControl::~PVControl()
{
}

void PVControl::configure(const ControlConfigType& tc)
{
  for(unsigned k=0; k<tc.npvControls(); k++)
    _channels.push_back(new ControlCA(*this,tc.pvControl(k)));
}

