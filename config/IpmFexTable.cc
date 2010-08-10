#include "IpmFexTable.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pdsdata/lusi/DiodeFexConfigV1.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using namespace Pds_ConfigDb;

static const int NCHAN = Pds::Lusi::IpmFexConfigV1::NCHANNELS;

using namespace Pds_ConfigDb;

IpmFexTable::IpmFexTable() :
  Parameter(NULL),
  _xscale("Horizontal Scale (um) ", 10, -1.e8, 1.e8),
  _yscale("Vertical Scale (um) ", 10, -1.e8, 1.e8)
{
  for(int i=0; i<MaxDiodes; i++)
    _diodes[i] = new DiodeFexItem;
}

IpmFexTable::~IpmFexTable() {}

void IpmFexTable::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(this);

  for(int i=0; i<MaxDiodes; i++)
    _diodes[i]->insert(_pList);
  _pList.insert(&_xscale);
  _pList.insert(&_yscale);
}

int IpmFexTable::pull(void* from) { // pull "from xtc"
  IpmFexConfigType& c = *new(from) IpmFexConfigType;
  for(int i=0; i<NCHAN; i++) 
    _diodes[i]->set(c.diode[i]);
  _xscale.value = c.xscale;
  _yscale.value = c.yscale;
  return sizeof(IpmFexConfigType);
}

int IpmFexTable::push(void* to) {
  Pds::Lusi::DiodeFexConfigV1 darray[NCHAN];
  for(int i=0; i<NCHAN; i++) {
    darray[i].base[0] = _diodes[i]->base0.value;
    darray[i].base[1] = _diodes[i]->base1.value;
    darray[i].base[2] = _diodes[i]->base2.value;
    darray[i].scale[0] = _diodes[i]->scale0.value;
    darray[i].scale[1] = _diodes[i]->scale1.value;
    darray[i].scale[2] = _diodes[i]->scale2.value;
  }
  *new(to) IpmFexConfigType(darray,
			    _xscale.value,
			    _yscale.value);
  return sizeof(IpmFexConfigType);
}

int IpmFexTable::dataSize() const { return sizeof(IpmFexConfigType); }

QLayout* IpmFexTable::initialize(QWidget* parent)
{
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  int row=0, column=1;
  layout->addWidget(new QLabel("Base0 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Base1 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Base2 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Scale0"), row, column++, align);
  layout->addWidget(new QLabel("Scale1"), row, column++, align);
  layout->addWidget(new QLabel("Scale2"), row, column++, align);
  for(int i=0; i<MaxDiodes; i++) {
    row++;
    layout->addWidget(new QLabel(QString("Channel %1").arg(i)), row, 0, align);
    _diodes[i]->initialize(parent, layout, row, 1);
  }
  layout->addLayout(_xscale.initialize(parent), ++row, 0, 1, 3);
  layout->addLayout(_yscale.initialize(parent), ++row, 0, 1, 3);
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

#include "Parameters.icc"
