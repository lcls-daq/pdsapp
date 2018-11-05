#include "pdsapp/python/FrameProcessor.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include <new>

using namespace Pds;

static unsigned _ievent = 0;

//
//  Insert a simulated transition
//
Dgram* FrameProcessor::insert(Dgram*              dg,
                              TransitionId::Value tr)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);

  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,0x1ffff,
                                          tr==TransitionId::L1Accept?_ievent++:0,
                                          0));

  new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), 
                           ProcInfo(Pds::Level::Segment,0,0));
  return dg;
}
