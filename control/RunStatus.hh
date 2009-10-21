#ifndef Pds_RunStatus_hh
#define Pds_RunStatus_hh

#include "pds/service/GenericPool.hh"
#include "pds/service/Timer.hh"
#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"
#include <QtGui/QGroupBox>

class QCounter;

namespace Pds {
  class PartitionControl;

  class RunStatus : public QGroupBox,
		    public Appliance,
		    public Timer,
		    public XtcIterator {
    Q_OBJECT
  public:
    RunStatus(QWidget*);
    ~RunStatus();
  public:
    virtual Transition* transitions(Transition*);
    virtual InDatagram* events     (InDatagram*);
  public:
    virtual void  expired();
    virtual Task* task();
    virtual unsigned duration() const;
    virtual unsigned repetitive() const;
  public:
    virtual int process(const Xtc&, InDatagramIterator*);
  private slots:
    void update_stats();
  signals:
    void changed();
  private:
    Task*     _task;
    GenericPool _pool;
    QCounter* _duration;
    QCounter* _events;
    QCounter* _damaged;
    QCounter* _bytes;
  };
};

#endif
