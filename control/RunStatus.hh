#ifndef Pds_RunStatus_hh
#define Pds_RunStatus_hh

#include "pds/service/GenericPool.hh"
#include "pds/service/Timer.hh"
#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"
#include <QtGui/QGroupBox>

class QPushButton;
class QPalette;

namespace Pds {
  class PartitionSelect;
  class QCounter;
  class DamageStats;

  class RunStatus : public QGroupBox,
		    public Appliance,
		    public Timer,
		    public XtcIterator {
    Q_OBJECT
  public:
    RunStatus(QWidget*, PartitionSelect&);
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
    void reset();
    void update_stats();
    void set_damage_alarm(bool);
  signals:
    void reset_s();
    void changed();
    void damage_alarm_changed(bool);
  private:
    Task*     _task;
    GenericPool _pool;
    QCounter* _duration;
    QCounter* _events;
    QCounter* _damaged;
    QCounter* _bytes;
    PartitionSelect& _partition;
    QPushButton* _detailsB;
    DamageStats* _details;
    bool         _alarm;
    QPalette*    _green;
    QPalette*    _red;
  };
};

#endif
