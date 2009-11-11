#ifndef Pds_StateSelect_hh
#define Pds_StateSelect_hh

#include "pds/utility/Appliance.hh"
#include <QtGui/QGroupBox>
#include <QtCore/QString>

class QPoint;
class QLabel;
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
  signals:
    void state_changed(QString);
    void allocated    ();
    void deallocated  ();
  public slots:
    void populate(QString);
    void selected(const QString&);
  private:
    PartitionControl& _control;
    QComboBox*        _select;
    QLabel*           _display;
    QPalette*         _green;
    QPalette*         _yellow;
  };
};

#endif
