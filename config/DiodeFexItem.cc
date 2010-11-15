#include "DiodeFexItem.hh"

#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/lusi/DiodeFexConfigV1.hh"

#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

static const float baseDef = 2.5;
static const float baseLo  = 0.0;
//static const float baseHi  = 3.3;
static const float baseHi  = 10; // inline baseline correction can produce output > 3.3
static const float scaleDef = 1.0;
static const float scaleLo  = -1.e20; 
static const float scaleHi  =  1.e20; 

DiodeFexItem::DiodeFexItem() :
  base0 (NULL, baseDef, baseLo, baseHi),
  base1 (NULL, baseDef, baseLo, baseHi),
  base2 (NULL, baseDef, baseLo, baseHi),
  scale0(NULL, scaleDef, scaleLo, scaleHi),
  scale1(NULL, scaleDef, scaleLo, scaleHi),
  scale2(NULL, scaleDef, scaleLo, scaleHi)
{}

DiodeFexItem::~DiodeFexItem() {}


void DiodeFexItem::set (const Pds::Lusi::DiodeFexConfigV1& c)
{
  base0.value = c.base[0];
  base1.value = c.base[1];
  base2.value = c.base[2];
  scale0.value = c.scale[0];
  scale1.value = c.scale[1];
  scale2.value = c.scale[2];
}

void DiodeFexItem::initialize(QWidget* parent,
			      QGridLayout* layout,
			      int row, int column)
{
  layout->addLayout(base0.initialize(parent), row, column++, Qt::AlignCenter);
  layout->addLayout(base1.initialize(parent), row, column++, Qt::AlignCenter);
  layout->addLayout(base2.initialize(parent), row, column++, Qt::AlignCenter);
  layout->addLayout(scale0.initialize(parent), row, column++, Qt::AlignCenter);
  layout->addLayout(scale1.initialize(parent), row, column++, Qt::AlignCenter);
  layout->addLayout(scale2.initialize(parent), row, column++, Qt::AlignCenter);
}

void DiodeFexItem::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&base0);
  pList.insert(&base1);
  pList.insert(&base2);
  pList.insert(&scale0);
  pList.insert(&scale1);
  pList.insert(&scale2);
}

#include "Parameters.icc"
