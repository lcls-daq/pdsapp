#include "PVControl.hh"

#include "pdsapp/control/PVRunnable.hh"
#include "pdsapp/control/EpicsCA.hh"

#include "pdsdata/control/PVControl.hh"

#include "db_access.h"

#define handle_type(ctype, stype, dtype) case ctype:	\
  { struct stype* ival = (struct stype*)dbr;		\
    dtype* inp  = &ival->value;				\
    dtype* outp = (dtype*)_pvdata;			\
    for(int k=0; k<nelem; k++) *outp++ = *inp++;	  \
    outp = (dtype*)_pvdata;					  \
    for(std::list<PvType>::const_iterator it = _channels.begin(); \
        it != _channels.end(); it++) { 				  \
      int idx = it->index();						\
      if (idx<0) idx=0;							\
      printf("handle_type %s idx %d val %g\n",#ctype,it->index(),it->value()); \
      outp[idx] = dtype(it->value());				\
    }								  \
  }

typedef Pds::ControlData::PVControl PvType;

namespace Pds {

  class ControlCA : public EpicsCA {
  public:
    ControlCA(PVControl& control,
	      const std::list<PvType>& channels) :
      EpicsCA(channels.front().name(),false),
      _control(control), _channels(channels),
      _connected(false), _runnable(false), _pvdata(0)
    {
    }
    virtual ~ControlCA() { printf("Deleting %p\n",_pvdata); if (_pvdata) delete[] _pvdata; }
  public:
    void connected   (bool c)
    {
      _connected = c;
      if (_runnable) {
	_runnable=false;
	_control.channel_changed();
      }
    }
    void  getData     (const void* dbr)  
    {
      int nelem = _channel.nelements();
      if (!_pvdata) {
	int sz = dbr_size_n(_channel.type(),nelem);
	_pvdata = new char[sz];
	printf("pvdata allocated @ %p sz %d\n",_pvdata,sz);
      }

      switch(_channel.type()) {
	handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
	handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
	handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
	handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
	handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
      default: printf("Unknown type %d\n", int(_channel.type())); break;
      }
      
    }
    void* putData     () { printf("putData called\n"); return _pvdata; }
    void  putStatus   (bool s) { 
      if (s!=_runnable) { 
	_runnable=s; 
	_control.channel_changed(); 
      }
    }
  public:
    bool runnable() const { return _runnable; }
  private:
    PVControl& _control;
    std::list<PvType> _channels;
    bool _connected;
    bool _runnable;
    char* _pvdata;
  };

};

using namespace Pds;

PVControl::PVControl (PVRunnable& report) :
  _report  (report),
  _runnable(false)
{
}

PVControl::~PVControl()
{
}

void PVControl::configure(const ControlConfigType& tc)
{
  if (tc.npvControls()==0) {
    if (!_runnable) 
      _report.runnable_change(_runnable=true);
  }
  else {
    if (_runnable)
      _report.runnable_change(_runnable=false);

    std::list<PvType> array;
    array.push_back(tc.pvControl(0));
    for(unsigned k=1; k<tc.npvControls(); k++) {
      const PvType& pv = tc.pvControl(k);
      if (strcmp(pv.name(),array.front().name())) {
	_channels.push_back(new ControlCA(*this,array));
	array.clear();
      }
      array.push_back(pv);
    }
    _channels.push_back(new ControlCA(*this,array));
  }
}

void PVControl::unconfigure()
{
  for(std::list<ControlCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    delete *iter;
  _channels.clear();
}

void PVControl::channel_changed()
{
  bool runnable = true;
  for(std::list<ControlCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    runnable &= (*iter)->runnable();
  if (runnable != _runnable) 
    _report.runnable_change(_runnable = runnable);
}

bool PVControl::runnable() const { return _runnable; }
