#include "DiodeFexTable.hh"
#include "DiodeFexItem.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using namespace Pds_ConfigDb;

DiodeFexTable::DiodeFexTable(unsigned n) :
  Parameter(NULL),
  _diode   (n)
{
}

DiodeFexTable::~DiodeFexTable() {}

void DiodeFexTable::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(this);
  _diode.insert(_pList);
}

void DiodeFexTable::pull(float* b, float* s) { // pull "from xtc"
  _diode.set(b,s);
}

void DiodeFexTable::push(float* b, float* s) {
  _diode.get(b,s);
}

QLayout* DiodeFexTable::initialize(QWidget* parent)
{
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  int row=0, column=0;
  for(unsigned i=0; i<_diode.nranges; i++)
    layout->addWidget(new QLabel(QString("Base%1 (V)").arg(i)), row++, column, align);
  for(unsigned i=0; i<_diode.nranges; i++)
    layout->addWidget(new QLabel(QString("Scale%1").arg(i)), row++, column, align);
  column++;
  row=0;
  _diode.initialize(parent, layout, row, column);
  return layout;
}

void DiodeFexTable::update()
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void DiodeFexTable::flush ()
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void DiodeFexTable::enable(bool e)
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->enable(e);
    p = p->forward();
  }
}
