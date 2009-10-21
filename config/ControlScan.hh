#ifndef Pds_ControlScan_hh
#define Pds_ControlScan_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"

#include <string>

class QCheckBox;
class QButtonGroup;
class QLineEdit;
class QString;

namespace Pds_ConfigDb {
  class Experiment;
  class TableEntry;
  class ControlScan : public QWidget {
    Q_OBJECT
  public:
    ControlScan(QWidget*, Experiment&);
    ~ControlScan();
  public:
    void set_run_type(const QString&);
  public slots:
    void update ();
    void details();
  signals:
    void created(int);
  private:
    void write();
    void read(const char*);
  private:
    int update_key();
  private:
    Experiment& _expt;
    std::string _run_type;
    QLineEdit* _steps       ;
    QLineEdit* _control_name;
    QLineEdit* _control_lo  ;
    QLineEdit* _control_hi  ;
    QLineEdit* _readback_name  ;
    QLineEdit* _readback_offset;
    QLineEdit* _readback_margin;
    QCheckBox* _settleB;
    QLineEdit* _settle_value ;
    QButtonGroup* _acqB;
    QLineEdit* _events_value ;
    QLineEdit* _time_value   ;
    SerializerDictionary _dict;
  };
};

#endif
