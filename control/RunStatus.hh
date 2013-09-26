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
  class PartitionControl;
  class PartitionSelect;
  class QCounter;
  class DamageStats;
  class L3TStats;

  class RunStatus : public QGroupBox,
                    public Appliance,
                    public Timer,
                    public PdsClient::XtcIterator {
    Q_OBJECT
  public:
    RunStatus(QWidget*, PartitionControl&, PartitionSelect&);
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
    void use_l3t(bool);
  signals:
    void reset_s();
    void changed();
    void damage_alarm_changed(bool);
    void l3t_used(bool);
    
  public:
    unsigned long long getEventNum();
    
  private:
    Task*     _task;
    GenericPool _pool;
    QCounter* _duration;
    QCounter* _events;
    QCounter* _damaged;
    QCounter* _bytes;
    L3TStats* _l3t;
    PartitionControl& _pcontrol;
    PartitionSelect& _partition;
    QPushButton* _detailsB;
    DamageStats* _details;
    bool         _alarm;
    QPalette*    _green;
    QPalette*    _red;
    unsigned  _prev_events;
    unsigned  _prev_damaged;
  };
};

#endif
