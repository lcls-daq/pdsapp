#ifndef Pds_RemoteSeqResponse_hh
#define Pds_RemoteSeqResponse_hh

#include "pdsdata/xtc/TransitionId.hh"

namespace Pds {
  class RemoteSeqResponse {
  public:
    RemoteSeqResponse() :
      _runinfo(0),
      _seqinfo(0)
    {}
    RemoteSeqResponse(unsigned exptnum,
		      unsigned runnum,
		      TransitionId::Value id,
		      unsigned damage) :
      _runinfo((exptnum&0xffff) | ((runnum&0xffff)<<16)),
      _seqinfo((id     &0xffff) | (damage ? 1<<31 : 0))
    {}
  public:
    unsigned exptnum() const { return (_runinfo>> 0)&0xffff; }
    unsigned runnum () const { return (_runinfo>>16)&0xffff; }
    TransitionId::Value id() const { return TransitionId::Value(_seqinfo&0xffff); }
    bool     damage    () const { return _seqinfo & 1<<31; }
  private:
    uint32_t _runinfo;
    uint32_t _seqinfo;
  };
};

#endif
