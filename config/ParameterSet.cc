#include "pdsapp/config/ParameterSet.hh"

#include "pdsapp/config/SubDialog.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>

using namespace Pds_ConfigDb;

ParameterSet::ParameterSet(const char* label,
			   Pds::LinkedList<Parameter>* array,
			   NumericInt<unsigned>&       nmembers) :
  QComboBox(0),
  Parameter(label),
  _array   (array),
  _nmembers(nmembers)
{}

ParameterSet::~ParameterSet()
{}

QLayout* ParameterSet::initialize(QWidget* parent)
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(_label));             
  this->setParent(parent);
  layout->addWidget(this);
  flush();
  layout->setContentsMargins(0,0,0,0);               
  QObject::connect(_nmembers._input, SIGNAL(editingFinished()),
		   this, SLOT(membersChanged()));
  QObject::connect(this, SIGNAL(activated(int)), 
		   this, SLOT(launch(int)));
  return layout;                                     
}

void ParameterSet::launch(int index)
{
  SubDialog* d = new SubDialog(this,
			       _array[index]);
  d->exec();
  delete d;
}

void ParameterSet::membersChanged() 
{
  flush();
}

void ParameterSet::update()
{
}

void ParameterSet::flush()
{
  unsigned n = this->count();
  while(_nmembers.value < n)
    this->removeItem(--n);
  while(_nmembers.value > n)
    this->addItem(QString::number(n++));
  QComboBox::update();
}

