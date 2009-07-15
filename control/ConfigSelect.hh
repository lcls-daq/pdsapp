#ifndef Pds_ConfigSelect_hh
#define Pds_ConfigSelect_hh

#include <QtGui/QGroupBox>

#include "pdsapp/config/Experiment.hh"

class QComboBox;
class QLineEdit;

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
    unsigned run_key() const;
  public slots:
    void set_run_type(const QString&);
    void set_run_key (const QString&);
    void update_run_types();
    void allocated  ();
    void deallocated();
  private:
    PartitionControl& _pcontrol;
    Pds_ConfigDb::Experiment _expt;
    QComboBox*        _runType;
    QLineEdit*        _runKey;
  };
};

#endif
