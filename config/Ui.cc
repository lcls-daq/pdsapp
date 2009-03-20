#include "pdsapp/config/Ui.hh"

#include "pdsapp/config/Transaction_Ui.hh"
#include "pdsapp/config/Experiment_Ui.hh"
#include "pdsapp/config/Devices_Ui.hh"

#include <QtGui/QVBoxLayout>

using namespace Pds_ConfigDb;

Ui::Ui(Experiment& expt) :
  QWidget(0)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(_transaction = new Transaction_Ui(this, expt));
  layout->addWidget(_experiment  = new Experiment_Ui (this, expt));
  layout->addWidget(_devices     = new Devices_Ui    (this, expt));
  setLayout(layout);

  connect(_transaction, SIGNAL(db_changed()), _experiment, SLOT(db_update()));
  connect(_transaction, SIGNAL(db_changed()), _devices   , SLOT(db_update()));
}

Ui::~Ui()
{
}
