#include "DiodeFexTable.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pdsdata/lusi/DiodeFexConfigV1.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using namespace Pds_ConfigDb;

using namespace Pds_ConfigDb;

DiodeFexTable::DiodeFexTable() :
  Parameter(NULL)
{
  _diode = new DiodeFexItem;
}

DiodeFexTable::~DiodeFexTable() {}

void DiodeFexTable::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(this);

  _diode->insert(_pList);
}

int DiodeFexTable::pull(void* from) { // pull "from xtc"
  DiodeFexConfigType& c = *new(from) DiodeFexConfigType;
  _diode->set(c);
  return sizeof(DiodeFexConfigType);
}

int DiodeFexTable::push(void* to) {
  Pds::Lusi::DiodeFexConfigV1 darray;
  darray.base[0] = _diode->base0.value;
  darray.base[1] = _diode->base1.value;
  darray.base[2] = _diode->base2.value;
  darray.scale[0] = _diode->scale0.value;
  darray.scale[1] = _diode->scale1.value;
  darray.scale[2] = _diode->scale2.value;

  *new(to) DiodeFexConfigType(darray);
  return sizeof(DiodeFexConfigType);
}

int DiodeFexTable::dataSize() const { return sizeof(DiodeFexConfigType); }

QLayout* DiodeFexTable::initialize(QWidget* parent)
{
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  int row=0, column=0;
  layout->addWidget(new QLabel("Base0 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Base1 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Base2 (V)"), row, column++, align);
  layout->addWidget(new QLabel("Scale0"), row, column++, align);
  layout->addWidget(new QLabel("Scale1"), row, column++, align);
  layout->addWidget(new QLabel("Scale2"), row, column++, align);
  row++;
  _diode->initialize(parent, layout, row, 0);
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
