#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/config/CspadConfigType.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/cspad/ConfigV1.hh"
#include "pdsdata/cspad/ElementV1.hh"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <vector>

using namespace Pds;

static std::vector<CspadConfigType> _config;
static std::vector<Pds::DetInfo>    _info;

//
//  Byte swap the payload (pixel data only) to fix the endianness
//
class myIter : public XtcIterator {
public:
  enum Status {Stop, Continue};
  myIter(Xtc* xtc) : XtcIterator(xtc) {}

  int process(Xtc* xtc) {
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    Level::Type level = xtc->src.level();
    if (level < 0 || level >= Level::NumberOfLevels ) {
      printf("Unsupported Level %d\n", (int) level);
    }
    else {
      switch (xtc->contains.id()) {
      case (TypeId::Id_Xtc) : {
	iterate(xtc);
	break;
      }
      case (TypeId::Id_CspadElement) : {
	if (xtc->damage.value()) break;
	Pds::CsPad::ElementV1* data = reinterpret_cast<Pds::CsPad::ElementV1*>(xtc->payload());
	for(unsigned i=0; i<_info.size(); i++)
	  if (_info[i] == info) {
	    const CspadConfigType& cfg = _config[i];
	    unsigned qmask = cfg.quadMask();
	    while(qmask) {
	      uint32_t* p=reinterpret_cast<uint32_t*>(data+1);
	      const uint32_t* const e=reinterpret_cast<const uint32_t*>(data->next(cfg));
	      while( p < e ) {
		unsigned v =
		  ((*p&0xff000000)>>24) |
		  ((*p&0x00ff0000)>> 8) |
		  ((*p&0x0000ff00)<< 8) |
		  ((*p&0x000000ff)<<24);
		*p++ = v;
	      }
	      qmask &= ~(1<<data->quad());
	      data = const_cast<Pds::CsPad::ElementV1*>(data->next(cfg));
	    }
	  }
	break;
      }
      case (TypeId::Id_CspadConfig) : {
	_config.push_back(*reinterpret_cast<const CspadConfigType*>(xtc->payload()));
	_info  .push_back(info);
	break;
      }
      default :
	break;
      }
    }
    return Continue;
  }
};

void CspadShuffle::shuffle(Datagram& dg) 
{
  if (dg.seq.service() == TransitionId::Configure) {
    _config.clear();
    _info  .clear();
    myIter iter(&(dg.xtc));
    iter.iterate();
  }
  else if (!_config.empty() && dg.seq.service() == TransitionId::L1Accept) {
    myIter iter(&(dg.xtc));
    iter.iterate();
  }
}
