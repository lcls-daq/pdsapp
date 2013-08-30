#include "DiodeFexItem.hh"

#include "pdsapp/config/ParameterSet.hh"

#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

static const float baseDef = 2.5;
static const float baseLo  = 0.0;
//static const float baseHi  = 3.3;
static const float baseHi  = 10; // inline baseline correction can produce output > 3.3
static const float scaleDef = 1.0;
static const float scaleLo  = -1.e20; 
static const float scaleHi  =  1.e20; 

DiodeFexItem::DiodeFexItem(unsigned n) : nranges(n)
{
  base  = new NumericFloat<float>[n];
  scale = new NumericFloat<float>[n];
  for(unsigned i=0; i<n; i++) {
    base [i] = NumericFloat<float>(NULL, baseDef , baseLo , baseHi );
    scale[i] = NumericFloat<float>(NULL, scaleDef, scaleLo, scaleHi);
  }
}

DiodeFexItem::~DiodeFexItem() 
{
  delete[] base;
  delete[] scale;
}


void DiodeFexItem::set (const float* b, const float* s)
{
  for(unsigned i=0; i<nranges; i++) {
    base [i].value = b[i];
    scale[i].value = s[i];
  }
}

void DiodeFexItem::get (float* b, float* s)
{
  for(unsigned i=0; i<nranges; i++) {
    b[i] = base [i].value;
    s[i] = scale[i].value;
  }
}

void DiodeFexItem::initialize(QWidget* parent,
                                 QGridLayout* layout,
                                 int row, int column)
{
  for(unsigned i=0; i<nranges; i++)
    layout->addLayout(base [i].initialize(parent), row++, column, Qt::AlignCenter);
  for(unsigned i=0; i<nranges; i++)
    layout->addLayout(scale[i].initialize(parent), row++, column, Qt::AlignCenter);
}

void DiodeFexItem::insert(Pds::LinkedList<Parameter>& pList) {
  for(unsigned i=0; i<nranges; i++) {
    pList.insert(&base [i]);
    pList.insert(&scale[i]);
  }
}

#include "Parameters.icc"
