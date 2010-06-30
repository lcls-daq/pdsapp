#ifndef Pds_StateSelect_hh
#define Pds_StateSelect_hh

#include "pds/utility/Appliance.hh"
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
  signals:
    void state_changed(QString);
    void configured   (bool);
  public slots:
    void populate(QString);
    void selected(const QString&);
    void set_record(bool);
  private:
    PartitionControl& _control;
    QCheckBox*        _record;
    QComboBox*        _select;
    QLabel*           _display;
    QPalette*         _green;
    QPalette*         _yellow;
  };
};

#endif
