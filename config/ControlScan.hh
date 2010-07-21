#ifndef Pds_ControlScan_hh
#define Pds_ControlScan_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"

#include <string>

class QCheckBox;
class QButtonGroup;
class QLineEdit;
class QString;
class QTabWidget;

namespace Pds_ConfigDb {
  class Experiment;
  class TableEntry;
  class PvScan;
  class EvrScan;
  class ControlScan : public QWidget {
    Q_OBJECT
  public:
    ControlScan(QWidget*, Experiment&);
    ~ControlScan();
  public:
    void set_run_type(const QString&);
    int update_key();
  public slots:
    void apply ();
    //    void details();
  signals:
    void deactivate ();
    void reconfigure();
  private:
    void write();
    void read(const char*);
  private:
    Experiment& _expt;
    std::string _run_type;
    QLineEdit*  _steps;
    QTabWidget* _tab;
    PvScan*     _pv;
    EvrScan*    _evr;
    QButtonGroup* _acqB;
    QLineEdit* _events_value ;
    QLineEdit* _time_value   ;
    SerializerDictionary _dict;
  };
};

#endif
