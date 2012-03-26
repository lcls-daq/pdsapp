#include "TimeTool.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/service/Routine.hh"
#include "pds/utility/NullServer.hh"
#include "pds/epicstools/PVWriter.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/opal1k/ConfigV1.hh"
#include "pdsdata/evr/DataV3.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include "pdsdata/encoder/ConfigV2.hh"
#include "pdsdata/encoder/DataV2.hh"
#include "alarm.h"
#include "pdsdata/epics/EpicsDbrTools.hh"
#include "pdsdata/epics/EpicsPvData.hh"

#include <math.h>
#include <string>

using std::string;

typedef Pds::Opal1k::ConfigV1 Opal1kConfig;
typedef Pds::EvrData::DataV3 EvrDataType;

using namespace Pds;
using Pds_Epics::PVWriter;

#include "TimeTool.icc"

static void _insert_pv(InDatagram* dg,
                       const Src&  src,
                       int         id,
                       const string& name)
{
  Pds::EpicsPvCtrlHeader payload(id,DBR_CTRL_DOUBLE,1,name.c_str());
  Xtc xtc(TypeId(TypeId::Id_Epics,1),src);
  xtc.extent += (sizeof(payload)+3)&(~3);
  dg->insert(xtc, &payload);
}

static void _insert_pv(InDatagram* dg,
                       const Src&  src,
                       int         id,
                       double      val)
{
  //  Pds::Epics::dbr_time_double v;
  dbr_time_double v;
  memset(&v,0,sizeof(v));
  v.value = val;
  Pds::EpicsPvTime<DBR_DOUBLE> payload(id,1,&v);
  Xtc xtc(TypeId(TypeId::Id_Epics,1),src);
  xtc.extent += (sizeof(payload)+3)&(~3);
  dg->insert(xtc, &payload);
}

namespace Pds {

  class FexApp : public XtcIterator {
  public:
    FexApp(const char* fname) : _fex(fname), _pv_writer(0) {}
    ~FexApp() {}
  public:
    Transition* transitions(Transition* tr) {
      if (tr->id()==TransitionId::Configure) {
        _fex.configure();
        _adjust_n = 0;
        _adjust_v = 0;
        if (_fex._adjust_stats) {
          _pv_writer = new PVWriter(_fex._adjust_pv.c_str());
        }
      }
      else if (tr->id()==TransitionId::Unconfigure) {
        if (_pv_writer) {
          delete _pv_writer;
          _pv_writer = 0;
        }
      }
      return tr;
    }
    InDatagram* events(InDatagram* dg) {
      switch(dg->datagram().seq.service()) {
      case TransitionId::L1Accept:
        { //  Add an encoder data object
          _fex.reset();

          _frame = 0;
          iterate(&dg->xtc);

          if (_frame)
            _fex.analyze(*_frame,_bykik,_no_laser);

          const Src& src = reinterpret_cast<Xtc*>(dg->xtc.payload())->src;
          Damage dmg(_fex.status() ? 0x4000 : 0);
          _insert_pv(dg, src, 0, _fex.amplitude());
          _insert_pv(dg, src, 1, _fex.filtered_position ());
          _insert_pv(dg, src, 2, _fex.filtered_pos_ps ());
          _insert_pv(dg, src, 3, _fex.filtered_fwhm ());
          _insert_pv(dg, src, 4, _fex.next_amplitude());
          _insert_pv(dg, src, 5, _fex.ref_amplitude());

          if (_fex.status()) {
            _adjust_v += _fex.filtered_pos_adj();
            _adjust_n++;
            if (_adjust_n==_fex._adjust_stats) {
              *reinterpret_cast<double*>(_pv_writer->data()) = 1.e3*_adjust_v/double(_adjust_n);
              _pv_writer->put();
              _adjust_v = 0;
              _adjust_n = 0;
            }
          }

          break; }
      case TransitionId::Configure:
        { //  Add an encoder config object
          const ProcInfo& info = reinterpret_cast<const ProcInfo&>(dg->xtc.src);
          DetInfo src  (info.processId(),
                       (DetInfo::Detector)((_fex._phy>>24)&0xff), ((_fex._phy>>16)&0xff),
                       (DetInfo::Device  )((_fex._phy>> 8)&0xff), ((_fex._phy>> 0)&0xff));
          _insert_pv(dg, src, 0, _fex.base_name()+":AMPL");
          _insert_pv(dg, src, 1, _fex.base_name()+":FLTPOS");
          _insert_pv(dg, src, 2, _fex.base_name()+":FLTPOS_PS");
          _insert_pv(dg, src, 3, _fex.base_name()+":FLTPOSFWHM");
          _insert_pv(dg, src, 4, _fex.base_name()+":AMPLNXT");
          _insert_pv(dg, src, 5, _fex.base_name()+":REFAMPL");
          break; }
      default:
        break;
      }

      return dg;
    }
    int process(Xtc* xtc) {
      if (xtc->contains.id()==TypeId::Id_Xtc) 
        iterate(xtc);
      else if (xtc->contains.id()==TypeId::Id_Frame) {
        _frame = reinterpret_cast<const Camera::FrameV1*>(xtc->payload());
      }
      return 1;
    }
    unsigned event_code_bykik   () const { return _fex._event_code_bykik; }
    unsigned event_code_no_laser() const { return _fex._event_code_no_laser; }
    void setup(bool bykik, bool no_laser) { _bykik=bykik; _no_laser=no_laser; }
  private:
    Pds_TimeTool::Fex _fex;
    const Camera::FrameV1* _frame;
    bool      _bykik;
    bool      _no_laser;
    PVWriter* _pv_writer;
    unsigned  _adjust_n;
    double    _adjust_v;
  };


  class QueuedFex : public Routine {
  public:
    QueuedFex(InDatagram* dg, TimeTool* app, FexApp* fex, 
              bool bykik, bool no_laser)
      : _dg(dg), _app(app), _fex(fex), 
        _bykik(bykik), _no_laser(no_laser), _sem(0) {}
    QueuedFex(InDatagram* dg, TimeTool* app, FexApp* fex, Semaphore* sem) 
      : _dg(dg), _app(app), _fex(fex), 
        _bykik(false), _no_laser(false), _sem(sem) {}
    ~QueuedFex() {}
  public:
    void routine() {

      _fex->setup(_bykik,_no_laser);
      _app->post(_fex->events(_dg));

      if (_sem) _sem->give();
      delete this;
    }
  private:
    InDatagram* _dg;
    TimeTool*   _app;
    FexApp*     _fex;
    bool        _bykik;
    bool        _no_laser;
    Semaphore*  _sem;
  };
};

TimeTool::TimeTool(const char* fname) :
  _task(new Task(TaskObject("ttool"))),
  _fex (new FexApp(fname)),
  _sem (Semaphore::EMPTY),
  _bykik(0),
  _no_laser(0)
{
}

TimeTool::~TimeTool()
{
  delete _fex;
}

Transition* TimeTool::transitions(Transition* tr)
{
  return _fex->transitions(tr);
}

InDatagram* TimeTool::events(InDatagram* dg)
{
  if (dg->datagram().seq.service()==TransitionId::L1Accept) {
    uint32_t b = 1 << (dg->datagram().seq.stamp().vector()&0x1f);
    bool bykik    = _bykik & b;
    bool no_laser = _no_laser & b;
    _task->call(new QueuedFex(dg, this, _fex, bykik, no_laser));
    _bykik    &= ~b;
    _no_laser &= ~b;
  }
  else {
    _task->call(new QueuedFex(dg, this, _fex, &_sem));
    _sem.take();
  }
  return (InDatagram*)Appliance::DontDelete;
}

Occurrence* TimeTool::occurrences(Occurrence* occ) {
  const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
  if (cmd.code == _fex->event_code_bykik())
    _bykik    |= 1<<(cmd.seq.stamp().vector()&0x1f);
  if (cmd.code == _fex->event_code_no_laser())
    _no_laser |= 1<<(cmd.seq.stamp().vector()&0x1f);
  return 0;
}
