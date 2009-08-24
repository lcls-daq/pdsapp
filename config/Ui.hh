#ifndef Pds_ConfigDb_Ui_hh
#define Pds_ConfigDb_Ui_hh

#include <QtGui/QWidget>

namespace Pds_ConfigDb {

  class Experiment;
  class Experiment_Ui;
  class Devices_Ui;

  class Ui : public QWidget {
  public:
    Ui(Experiment&,bool edit);
    ~Ui();
  };
};

#endif
