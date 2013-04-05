#include "pdsapp/config/Ui.hh"

#include "pdsapp/config/Info_Ui.hh"
#include "pdsapp/config/Transaction_Ui.hh"
#include "pdsapp/config/Experiment_Ui.hh"
#include "pdsapp/config/Devices_Ui.hh"

#include <QtGui/QVBoxLayout>

using namespace Pds_ConfigDb;

Ui::Ui(Experiment& expt, bool edit) :
  QWidget(0)
{
  Info_Ui*        info        = new Info_Ui       (this, expt);
  Transaction_Ui* transaction = new Transaction_Ui(this, expt, edit);
  Experiment_Ui*  experiment  = new Experiment_Ui (this, expt, edit);
  Devices_Ui*     devices     = new Devices_Ui    (this, expt, edit);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(info);
  layout->addWidget(transaction);
  layout->addWidget(experiment);
  layout->addWidget(devices);
  setLayout(layout);

  connect(transaction, SIGNAL(db_changed()), experiment, SLOT(db_update()));
  connect(transaction, SIGNAL(db_changed()), devices   , SLOT(db_update()));
  connect(devices    , SIGNAL(db_changed()), experiment, SLOT(db_update()));
}

Ui::~Ui()
{
}
