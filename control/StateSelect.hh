#ifndef Pds_StateSelect_hh
#define Pds_StateSelect_hh

#include "pds/utility/Appliance.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include <QtGui/QGroupBox>

class QPoint;
class QString;
class QLabel;
class QComboBox;
class QPalette;

namespace Pds {
  class PartitionControl;
  class MainWindow;
 
  class StateSelect : public QGroupBox,
		      public Appliance {
    Q_OBJECT
  public:
    StateSelect(MainWindow*,
		PartitionControl&,
        OfflineClient*);
    ~StateSelect();
  public:
    virtual Transition* transitions(Transition*);
    virtual InDatagram* events     (InDatagram*);
  signals:
    void state_changed();
    void allocated    ();
    void deallocated  ();
  public slots:
    void populate();
    void selected(const QString&);
  private:
    PartitionControl& _control;
    OfflineClient*    _offlineclient;
    QComboBox*        _select;
    QLabel*           _display;
    QPalette*         _green;
    QPalette*         _yellow;
    MainWindow&       _w;
  };
};

#endif
