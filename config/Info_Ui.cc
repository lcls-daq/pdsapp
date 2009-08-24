#include "pdsapp/config/Info_Ui.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/ListUi.hh"

#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>

using namespace Pds_ConfigDb;

Info_Ui::Info_Ui(QWidget* parent,
		 Experiment& expt) :
  QGroupBox("Database Info", parent),
  _expt(expt)
{
  QHBoxLayout* layout  = new QHBoxLayout(this);
  QPushButton* current = new QPushButton("List Current Keys", this);
  QPushButton* browse  = new QPushButton("Browse Keys", this);
  layout->addWidget(current);
  layout->addWidget(browse );

  connect(current, SIGNAL(clicked()), this, SLOT(db_current()));
  connect(browse , SIGNAL(clicked()), this, SLOT(db_browse ()));
  setLayout(layout);
}

Info_Ui::~Info_Ui()
{
}

void Info_Ui::db_current()
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

void Info_Ui::db_browse()
{
  (new ListUi(_expt.path()))->show();
}
