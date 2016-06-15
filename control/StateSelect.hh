#ifndef Pds_StateSelect_hh
#define Pds_StateSelect_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Semaphore.hh"
#include <QtGui/QGroupBox>
#include <QtCore/QString>

class QPoint;
class QLabel;
class QCheckBox;
class QComboBox;
class QPalette;

namespace Pds {
  class PartitionControl;
 
  class StateSelect : public QGroupBox,
		      public Appliance {
    Q_OBJECT
  public:
    StateSelect(QWidget*,
		PartitionControl&);
    ~StateSelect();
  public:
    virtual Transition* transitions(Transition*);
    virtual InDatagram* events     (InDatagram*);
  public:
    bool control_enabled() const;
    void enable_control();
    void disable_control();
    bool record_state() const;
    void set_record_state(bool);
    void qtsync();
  signals:
    void remote_record(bool);
    void state_changed(QString);
    void configured   (bool);
    void _enable_control(bool);
    void _qtsync();
  public slots:
    void populate(QString);
    void selected(const QString&);
    void set_record(bool);
    void unblock();
  private:
    PartitionControl& _control;
    QCheckBox*        _record;
    QComboBox*        _select;
    QLabel*           _display;
    QPalette*         _green;
    QPalette*         _yellow;
    Semaphore         _sem;
    bool              _manual;
  };
};

#endif
