#include "pdsapp/config/ParameterSet.hh"

#include "pdsapp/config/ParameterCount.hh"
#include "pdsapp/config/SubDialog.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtCore/QString>
#include <string.h>

using namespace Pds_ConfigDb;

ParameterSet::ParameterSet(const char* label,
			   Pds::LinkedList<Parameter>* array,
			   ParameterCount& count) :
  Parameter(label),
  _array   (array),
  _count   (count),
  _qset    (new ParameterSetQ(*this)),
  _index   (0)
{
  strcpy(_name, "");
}

ParameterSet::~ParameterSet()
{}

QLayout* ParameterSet::initialize(QWidget* parent)
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(_label));   
  _box = new QComboBox(parent);
  layout->addWidget(_box);
  flush();
  layout->setContentsMargins(0,0,0,0);               
  if (Parameter::allowEdit())
    _count.connect(*this);
  QObject::connect(_box, SIGNAL(activated(int)), 
		   _qset, SLOT(launch(int)));
  return layout;                                     
}

void ParameterSet::name(char* n) {
  strncpy(_name, n, 80);
}

void ParameterSet::launch(int index)
{
  QWidget*  tbi = insertWidgetAtLaunch(index);
  SubDialog* d = new SubDialog(_box,
			       _array[index], tbi);
  char foo[strlen(_name)+20];
  sprintf(foo, "%s %d", _name, index);
  QString* title = new QString(foo);
  d->setWindowTitle(*title);
  _index = index;
  d->exec();
  delete title;
  delete d;
}

void ParameterSet::membersChanged() 
{
  flush();
}

void ParameterSet::update()
{
}

void ParameterSet::flush ()
{
  unsigned n = _box->count();
  unsigned cnt = _count.count();
  while(cnt < n)
    _box->removeItem(--n);
  while(cnt > n)
    _box->addItem(QString::number(n++));
  _box->update();
}

void ParameterSet::enable(bool v)
{
  _box->setEnabled(allowEdit() && v);
}


ParameterSetQ::ParameterSetQ(ParameterSet& p) : _pset(p) {}

ParameterSetQ::~ParameterSetQ() {}

void ParameterSetQ::launch(int i) { _pset.launch(i); }
void ParameterSetQ::membersChanged() { _pset.membersChanged(); }
