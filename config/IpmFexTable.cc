#include "IpmFexTable.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using namespace Pds_ConfigDb;

using namespace Pds_ConfigDb;

IpmFexTable::IpmFexTable(unsigned n) :
  Parameter(NULL),
  _xscale("Horizontal Scale (um) ", 10, -1.e8, 1.e8),
  _yscale("Vertical Scale (um) ", 10, -1.e8, 1.e8)
{
  for(int i=0; i<MaxDiodes; i++)
    _diodes[i] = new DiodeFexItem(n);
}

IpmFexTable::~IpmFexTable() {}

void IpmFexTable::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(this);

  for(int i=0; i<MaxDiodes; i++)
    _diodes[i]->insert(_pList);
  _pList.insert(&_xscale);
  _pList.insert(&_yscale);
}

float IpmFexTable::xscale() const { return _xscale.value; }
float IpmFexTable::yscale() const { return _yscale.value; }
void IpmFexTable::get(int ch, float* b, float* s) const { _diodes[ch]->get(b,s);}

void IpmFexTable::xscale(float v) { _xscale.value=v; }
void IpmFexTable::yscale(float v) { _yscale.value=v; }
void IpmFexTable::set(int ch, const float* b, const float* s){ _diodes[ch]->set(b,s);}

QLayout* IpmFexTable::initialize(QWidget* parent)
{
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  int row=1, column=0;
  for(unsigned i=0; i<_diodes[0]->nranges; i++)
    layout->addWidget(new QLabel(QString("Base%1 (V)").arg(i)), row++, column, align);
  for(unsigned i=0; i<_diodes[0]->nranges; i++)
    layout->addWidget(new QLabel(QString("Scale%1").arg(i)), row++, column, align);
  column=1;
  for(unsigned i=0; i<MaxDiodes; i++) {
    layout->addWidget(new QLabel(QString("Channel %1").arg(i)), 0, column, align);
    _diodes[i]->initialize(parent, layout, 1, column);
    column++;
  }
  layout->addLayout(_xscale.initialize(parent), row++, 0, 1, 3);
  layout->addLayout(_yscale.initialize(parent), row++, 0, 1, 3);
  return layout;
}

void IpmFexTable::update()
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void IpmFexTable::flush ()
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void IpmFexTable::enable(bool e)
{
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->enable(e);
    p = p->forward();
  }
}


