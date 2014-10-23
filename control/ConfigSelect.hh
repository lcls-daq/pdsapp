#ifndef Pds_ConfigSelect_hh
#define Pds_ConfigSelect_hh

#include <QtGui/QGroupBox>

#include "pdsapp/config/Experiment.hh"

#include <pthread.h>

class QCheckBox;
class QComboBox;
class QPushButton;
class QMessageBox;

namespace Pds_ConfigDb {
  class Reconfig_Ui;
  class ControlScan;
};

namespace Pds {
  class PartitionControl;
  class Sequencer;
  class ConfigSelect : public QGroupBox {
    Q_OBJECT
  public:
    ConfigSelect(QWidget*          parent,
     PartitionControl& control,
     const char*       db_path);
    ~ConfigSelect();
  public:
    void attach();
    string getType();
    void enable_control(bool);
    bool controlpvs() const;
    void configured_   (bool);
  public slots:
    void attached();
    void set_run_type(const QString&); // a run type has been selected
    void update      ();  // the latest key for the selected run type has changed
    void configured     (bool);
    void enable_scan    (bool);
    void enable_control_(bool);    
    void edit_config    ();
  private:  
    void _open_db      ();
    void _read_db      ();
    void _readSettings ();
    void _writeSettings();
  signals:
    void _attached     ();
    void control_enabled(bool);
    void assert_message(const QString&,bool);
    void aliases_required();
  private:
    PartitionControl&          _pcontrol;
    const char*                _db_path;
    Pds_ConfigDb::Experiment*  _expt;
    Pds_ConfigDb::Reconfig_Ui* _reconfig;
    Pds_ConfigDb::ControlScan* _scan;
    QComboBox*                 _runType;
    unsigned                   _run_key;
    QPushButton*               _bEdit;
    QPushButton*               _bScan;
    QCheckBox*                 _cSeq;
    QComboBox*                 _bSeq;
    Sequencer*                 _seq;

    bool                       _control_busy;
    pthread_mutex_t            _control_mutex;
    pthread_cond_t             _control_cond;

    QMessageBox*               _dblock;
  };
};

#endif
