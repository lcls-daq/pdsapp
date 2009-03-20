#include "pdsapp/config/Transaction_Ui.hh"

#include "pdsapp/config/Experiment.hh"

#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

using namespace Pds_ConfigDb;

Transaction_Ui::Transaction_Ui(QWidget* parent,
			       Experiment& expt) :
  QGroupBox("Database Transaction", parent),
  _expt(expt)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  QPushButton* clear  = new QPushButton("Clear" , this);
  QPushButton* commit = new QPushButton("Commit", this);
  QPushButton* update = new QPushButton("Update Keys", this);
  layout->addWidget(clear);
  layout->addWidget(commit);
  layout->addWidget(update);
  connect(clear , SIGNAL(clicked()), this, SLOT(db_clear() ));
  connect(commit, SIGNAL(clicked()), this, SLOT(db_commit()));
  connect(update, SIGNAL(clicked()), this, SLOT(db_update()));
  setLayout(layout);

  _expt.read();
}

Transaction_Ui::~Transaction_Ui()
{
}

void Transaction_Ui::db_clear()
{
  _expt.read();
  emit db_changed();
}

void Transaction_Ui::db_commit()
{
  _expt.write();
}

void Transaction_Ui::db_update()
{
  _expt.update_keys();
  emit db_changed();
}
