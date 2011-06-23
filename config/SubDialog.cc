#include "pdsapp/config/SubDialog.hh"

#include "pdsapp/config/Parameters.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

#include <sys/stat.h>

using namespace Pds_ConfigDb;

SubDialog::SubDialog(QWidget* parent,
		     Pds::LinkedList<Parameter>& pList, QWidget* tbi) :
  QDialog(parent),
  _pList (pList)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    layout->addLayout(p->initialize(this));
    p = p->forward();
  }
  if (tbi) layout->addWidget(tbi);
  QPushButton* bReturn = new QPushButton("Return",this);
  layout->addWidget(bReturn);
  connect(bReturn, SIGNAL(clicked()), this, SLOT(_return()));

  setLayout(layout);
}

SubDialog::~SubDialog() {}

void SubDialog::_return()
{
  Pds::LinkedList<Parameter>& pList(_pList);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    p->update();
    p = p->forward();
  }
  accept();
}
