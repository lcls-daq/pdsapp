#ifndef Pds_ConfigSelect_hh
#define Pds_ConfigSelect_hh

#include <QtGui/QGroupBox>

#include "pdsapp/config/Experiment.hh"

class QComboBox;
class QPushButton;

namespace Pds_ConfigDb {
  class Reconfig_Ui;
  class ControlScan;
};

namespace Pds {
  class PartitionControl;
  class ConfigSelect : public QGroupBox {
    Q_OBJECT
  public:
    ConfigSelect(QWidget*          parent,
		 PartitionControl& control,
		 const char*       db_path);
    ~ConfigSelect();
  public:
    void read_db();
  public slots:
    void set_run_type(const QString&); // a run type has been selected
    void update      ();  // the latest key for the selected run type has changed
    void configured  (bool);
    void enable_scan (bool);
    void enable_control(bool);
  private:
    void _readSettings ();
    void _writeSettings();
  private:
    PartitionControl&          _pcontrol;
    Pds_ConfigDb::Experiment   _expt;
    Pds_ConfigDb::Reconfig_Ui* _reconfig;
    Pds_ConfigDb::ControlScan* _scan;
    QComboBox*                 _runType;
    unsigned                   _run_key;
    bool                       _scanIsActive;
    QPushButton*               _bEdit;
    QPushButton*               _bScan;
  };
};

#endif
