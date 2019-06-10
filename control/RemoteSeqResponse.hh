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
    RemoteSeqResponse(const char* expname,
		      unsigned runnum,
		      TransitionId::Value id,
		      unsigned damage) :
      _runinfo((0&0xffff) | ((runnum&0xffff)<<16)),
      _seqinfo((id     &0xffff) | (damage ? 1<<31 : 0))
    { strncpy(_expname, expname, MaxExpName-1); }
  public:
    const char* exptname() const { return _expname; }
    unsigned runnum () const { return (_runinfo>>16)&0xffff; }
    TransitionId::Value id() const { return TransitionId::Value(_seqinfo&0xffff); }
    bool     damage    () const { return _seqinfo & 1<<31; }
  private:
    uint32_t _runinfo;
    uint32_t _seqinfo;
    static const unsigned MaxExpName=32;
    char     _expname[MaxExpName];
  };
};

#endif
