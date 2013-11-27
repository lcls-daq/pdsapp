//#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>

#include "pdsapp/blv/EvrBldManager.hh" 

#include "pds/client/Action.hh" 
#include "pdsdata/xtc/DetInfo.hh"

//#include "EvrConfigType.hh" 
#include "pds/config/EvrConfigType.hh" 

#include <string>

//#define DBUG

using namespace Pds;

static const unsigned TERMINATOR        = 1;

static EvrBldManager* evrBldMgrGlobal = NULL;

//EVR Signal Handler
extern "C" {
  void evrsa_sig_handler(int parm) { 
  if(evrBldMgrGlobal)	  
    evrBldMgrGlobal->handleEvrIrq();  
  }
} 


class EvrBldMapAction : public Action {
public:
  EvrBldMapAction(EvrBldManager* evrBldMgr) :
    _evrBldMgr(evrBldMgr)  {}
  Transition* fire(Transition* tr) {   	  
    _evrBldMgr->allocate(tr); 
    return tr;
  }
private:
  EvrBldManager* _evrBldMgr;
};

class EvrBldConfigAction : public Action {
public:
  EvrBldConfigAction(EvrBldManager* evrBldMgr) :
    _evrBldMgr(evrBldMgr)  {}
  Transition* fire(Transition* tr) {   	  
    _evrBldMgr->configure(tr); 
    return tr;
  }
private:
  EvrBldManager* _evrBldMgr;
};

class EvrBldEnableAction : public Action {
public:
  EvrBldEnableAction(EvrBldManager* evrBldMgr): _evrBldMgr(evrBldMgr) {}
 
  Transition* fire(Transition* tr) {
    _evrBldMgr->enable();
    _evrBldMgr->reset();
    _evrBldMgr->start();
    return tr;
  }

private:
  EvrBldManager* _evrBldMgr;  
};

class EvrBldDisableAction : public Action {
public:
  EvrBldDisableAction(EvrBldManager* evrBldMgr): _evrBldMgr(evrBldMgr) {}

  Transition* fire(Transition* tr) {
    _evrBldMgr->stop();
    _evrBldMgr->disable();
    return tr;
  }
private:
  EvrBldManager* _evrBldMgr;  
};


EvrBldManager::EvrBldManager(const DetInfo&        src, 
                             const char*           evr_id,
                             const std::list<int>& write_fd) :
  _erInfo      ((std::string("/dev/er")+evr_id[0]+'3').c_str()),
  _er          (_erInfo.board()), 
  _write_fd    (write_fd),
  _cfg         (*new EvrCfgClient(src,"")),
  _evtCounter  (0),
  _configBuffer(new char[0x10000]),
  _hsignal     ("EvrSignal")
{
#ifdef DBUG
  _tsignal.tv_sec = _tsignal.tv_nsec = 0;
#endif

  _er.IrqAssignHandler(_erInfo.filedes(), &evrsa_sig_handler);  
  evrBldMgrGlobal = this;
  
  _fsm.callback(TransitionId::Map      ,new EvrBldMapAction(this));
  _fsm.callback(TransitionId::Configure,new EvrBldConfigAction(this));
  _fsm.callback(TransitionId::L1Accept, new EvrBldEnableAction(this));
  _fsm.callback(TransitionId::Disable,  new EvrBldDisableAction(this));
}

EvrBldManager::~EvrBldManager()
{
  delete[] _configBuffer;
}

void EvrBldManager::start() {
  unsigned ram = 0;
  _er.MapRamEnable(ram, 1);
};

void EvrBldManager::stop() { 
  // switch to the "dummy" map ram 
  unsigned dummyram=1;
  _er.MapRamEnable(dummyram,1);
  _er.ClearFIFO();  
}

void EvrBldManager::enable() {
  printf("### FLAGS : FIFO_FULL(%x), EVENT(%x)\n",EVR_IRQFLAG_FIFOFULL, EVR_IRQFLAG_EVENT);

  int flags;
  flags = _er.GetIrqFlags();
  printf("### Get IRQ Flags 0x%x\n",flags);

  //  _er.ClearIrqFlags(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT | EVR_IRQFLAG_FIFOFULL);
  _er.ClearIrqFlags(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  _er.ClearFIFO();

  flags = _er.GetIrqFlags();
  printf("### Get IRQ Flags 0x%x\n",flags);

  int test = _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  printf("### Enabled EVR IRQ Flags 0x%x\n",test);
  _er.EnableFIFO(1);
  _er.Enable(1);
}

void EvrBldManager::disable() {
  _er.IrqEnable(0);
  _er.Enable(0);
  _er.EnableFIFO(0);
}

void EvrBldManager::reset() { _evtCounter = 0; }

void EvrBldManager::handleEvrIrq() {

#ifdef DBUG
  timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  if (_tsignal.tv_sec)
    _hsignal.accumulate(ts,_tsignal);
  _tsignal = ts;
#endif

  int flags = _er.GetIrqFlags();
  if (flags & EVR_IRQFLAG_EVENT) {

    _er.ClearIrqFlags(EVR_IRQFLAG_EVENT);

    FIFOEvent fe;
    unsigned n=0;
    while( !_er.GetFIFOEvent(&fe))
    {
      n++;
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts); 
      ClockTime ctime(ts.tv_sec, ts.tv_nsec);
      TimeStamp stamp(fe.TimestampLow, fe.TimestampHigh, _evtCounter);
      Sequence seq(Sequence::Event, TransitionId::L1Accept, ctime, stamp);
      EvrDatagram datagram(seq, _evtCounter++);	
      for(std::list<int>::const_iterator it=_write_fd.begin(); it!=_write_fd.end(); it++) {
#ifdef DBUG
        printf("EvrBldMgr::handle %08x.%08x to %d\n",
               reinterpret_cast<const uint32_t*>(&seq.stamp())[0],
               reinterpret_cast<const uint32_t*>(&seq.stamp())[1],
               *it);
#endif
        ::write(*it, (char*)&datagram, sizeof(EvrDatagram));
      }
#ifdef DBUG
      _hsignal.print(_evtCounter);
#endif
    } 

    if(flags & EVR_IRQFLAG_FIFOFULL) {
      printf("*** Received: EVENT & FIFO_FULL IRQ FLAG : 0x%x : %d events in FIFO [%d]\n",
             flags,n,_evtCounter);
      _er.ClearIrqFlags(EVR_IRQFLAG_FIFOFULL);	  
    }

  } else { 
    printf("*** Spurious interrupt, IRQ flags = 0x%x \n",flags);  
  } 

  int fdEr = _erInfo.filedes();
  _er.IrqHandled(fdEr); 
}

void EvrBldManager::allocate(Transition* tr) 
{
  const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
  _cfg.initialize(alloc.allocation());
}

void EvrBldManager::configure(Transition* tr) 
{ 
  bool slacEvr     = reinterpret_cast<uint32_t*>(&_er)[11] == 0x1f;

  _er.Reset();

  // Problem in Reset() function: It doesn't reset the set and clear masks 
  // workaround: manually call the clear function to set and clear all masks
  for (unsigned ram=0;ram<2;ram++) {
    for (unsigned iopcode=0;iopcode<=EVR_MAX_EVENT_CODE;iopcode++) {
      for (unsigned jSetClear=0;jSetClear<EVR_MAX_PULSES;jSetClear++)
        _er.ClearPulseMap(ram, iopcode, jSetClear, jSetClear, jSetClear);
    }
  }    

  if (_cfg.fetch(*tr, _evrConfigType, _configBuffer) <= 0)
    return;

  const EvrConfigType& cfg = 
    *reinterpret_cast<const EvrConfigType*>(_configBuffer);

  // Another problem: The output mapping to pulses never clears
  // workaround: Set output map to the last pulse (just cleared)
  unsigned omask = 0;
  for (unsigned k = 0; k < cfg.noutputs(); k++)
    omask |= 1<<cfg.output_maps()[k].conn_id();
  omask = ~omask;
  
  for (unsigned k = 0; k < EVR_MAX_UNIVOUT_MAP; k++) {
    if ((omask >> k)&1 && !(k>9 && !slacEvr))
      _er.SetUnivOutMap(k, EVR_MAX_PULSES-1);
  }

  // setup map ram
  int ram = 0;
  int enable = 1;

  for (unsigned k = 0; k < cfg.npulses(); k++)
    {
      const PulseType & pc = cfg.pulses()[k];
      _er.SetPulseProperties(
                             pc.pulseId(),
                             pc.polarity(),
                             1, // Enable reset from event code
                             1, // Enable set from event code
                             1, // Enable trigger from event code
                             1  // Enable pulse
                             );

      _er.SetPulseParams(pc.pulseId(), pc.prescale(), pc.delay(), pc.width());

      printf("::pulse %d :%d %c %d/%d/%d\n",
             k, pc.pulseId(), pc.polarity() ? '-':'+',
             pc.prescale(), pc.delay(), pc.width());
    }

  _er.DumpPulses(cfg.npulses());

  for (unsigned k = 0; k < cfg.noutputs(); k++)
    {
      const OutputMapType & map = cfg.output_maps()[k];
      switch (map.conn())
        {
        case OutputMapType::FrontPanel:
          _er.SetFPOutMap(map.conn_id(), map.value());
          break;
        case OutputMapType::UnivIO:
          _er.SetUnivOutMap(map.conn_id(), map.value());
          break;
        }

      printf("output %d : %d %x\n", k, map.conn_id(), map.value());
    }


  /*
   * enable event codes, and setup each event code's pulse mapping
   */
  for (unsigned int uEventIndex = 0; uEventIndex < cfg.neventcodes(); uEventIndex++ )
    {
      const EventCodeType& eventCode = cfg.eventcodes()[uEventIndex];

      _er.SetFIFOEvent(ram, eventCode.code(), enable);

      unsigned int  uPulseBit     = 0x0001;
      uint32_t      u32TotalMask  = ( eventCode.maskTrigger() | eventCode.maskSet() | eventCode.maskClear() );

      for ( int iPulseIndex = 0; iPulseIndex < EVR_MAX_PULSES; iPulseIndex++, uPulseBit <<= 1 )
        {
          if ( (u32TotalMask & uPulseBit) == 0 ) continue;

          _er.SetPulseMap(ram, eventCode.code(),
                          ((eventCode.maskTrigger() & uPulseBit) != 0 ? iPulseIndex : -1 ),
                          ((eventCode.maskSet()     & uPulseBit) != 0 ? iPulseIndex : -1 ),
                          ((eventCode.maskClear()   & uPulseBit) != 0 ? iPulseIndex : -1 )
                          );
        }

      printf("event %d : %d %x/%x/%x\n",
             uEventIndex, eventCode.code(),
             eventCode.maskTrigger(),
             eventCode.maskSet(),
             eventCode.maskClear());
    }

  //  _er.SetFIFOEvent(ram, TERMINATOR, enable);

  _er.IrqEnable(0);
  _er.Enable(0);
  _er.EnableFIFO(0);
  _er.ClearFIFO();
  _er.ClearIrqFlags(0xffffffff);

  unsigned dummyram = 1;
  _er.MapRamEnable(dummyram, 1);
}
