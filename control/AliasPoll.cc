#include "pdsapp/control/AliasPoll.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/collection/Node.hh"
#include "pds/collection/AliasReply.hh"
#include "pds/collection/PingReply.hh"

using namespace Pds;

AliasPoll::AliasPoll(PartitionControl& control) :
  _control(control)
{
  _control.platform_rollcall(this);
}

AliasPoll::~AliasPoll()
{
  _control.platform_rollcall(0);
}

void        AliasPoll::available(const Node& hdr, const PingReply& msg)
{
  if (hdr.level()==Level::Segment) {
    for(unsigned i=0; i<msg.nsources(); i++) {
      if (hdr.triggered()) 
	_evrio.insert(hdr.evr_module(),
		      hdr.evr_channel(),
		      static_cast<const DetInfo&>(msg.source(i)));
    }
  }
}

void        AliasPoll::aliasCollect(const Node& hdr, const AliasReply& msg)
{
  if (hdr.level() == Level::Segment) {
    int count = (int)msg.naliases();
    for (int ix=0; ix < count; ix++) {
      _aliases.insert(msg.alias(ix));
    }
  }
}

const AliasFactory&    AliasPoll::aliases() const { return _aliases; }
const EvrIOFactory&    AliasPoll::evrio  () const { return _evrio  ; }
