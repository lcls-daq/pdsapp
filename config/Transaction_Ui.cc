#include "pdsapp/config/Transaction_Ui.hh"

#include "pdsapp/config/Experiment.hh"

#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>

using namespace Pds_ConfigDb;

Transaction_Ui::Transaction_Ui(QWidget* parent,
			       Experiment& expt) :
  QGroupBox("Database Transaction", parent),
  _expt(expt)
{
  QHBoxLayout* layout  = new QHBoxLayout(this);
  QPushButton* clear   = new QPushButton("Clear" , this);
  QPushButton* commit  = new QPushButton("Commit", this);
  QPushButton* update  = new QPushButton("Update Keys", this);
  QPushButton* current = new QPushButton("Current Keys", this);
  layout->addWidget(clear);
  layout->addWidget(commit);
  layout->addWidget(update);
  layout->addWidget(current);
  connect(clear  , SIGNAL(clicked()), this, SLOT(db_clear() ));
  connect(commit , SIGNAL(clicked()), this, SLOT(db_commit()));
  connect(update , SIGNAL(clicked()), this, SLOT(db_update()));
  connect(current, SIGNAL(clicked()), this, SLOT(db_current()));
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
  _expt.write();
  emit db_changed();
}

void Transaction_Ui::db_current()
{
  QString message;
  list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter) {
    message += iter->name().c_str();
    message += "\t" ;
    message += iter->key().c_str();
    message += "\n";
  }
  QMessageBox::information(this, "Current Keys", message);
}
