#ifndef Pds_ConfigDb_Ui_hh
#define Pds_ConfigDb_Ui_hh

#include <QtGui/QWidget>

namespace Pds_ConfigDb {

  class Experiment;
  class Transaction_Ui;
  class Experiment_Ui;
  class Devices_Ui;

  class Ui : public QWidget {
  public:
    Ui(Experiment&);
    ~Ui();
  private:
    Transaction_Ui* _transaction;
    Experiment_Ui*  _experiment;
    Devices_Ui*     _devices;
  };
};

#endif
