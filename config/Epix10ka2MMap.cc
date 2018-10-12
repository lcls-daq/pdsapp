#include "pdsapp/config/Epix10ka2MMap.hh"
#include "pdsapp/config/EpixSector.hh"

#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

Epix10ka2MMap::Epix10ka2MMap(unsigned g, bool e) : QWidget(0)
{
  QGridLayout* ql = new QGridLayout;
  ql->addWidget(_sectors[3] = new EpixSector(3,g,e),0,0,::Qt::AlignBottom|::Qt::AlignRight);
  ql->addWidget(_sectors[0] = new EpixSector(0,g,e),0,1,::Qt::AlignBottom|::Qt::AlignLeft);
  ql->addWidget(_sectors[2] = new EpixSector(2,g,e),1,0,::Qt::AlignTop   |::Qt::AlignRight);
  ql->addWidget(_sectors[1] = new EpixSector(1,g,e),1,1,::Qt::AlignTop   |::Qt::AlignLeft);
  setLayout(ql);

  if (e) {
    ::QObject::connect(_sectors[0], SIGNAL(changed()), this, SLOT(setQ0()));
    ::QObject::connect(_sectors[1], SIGNAL(changed()), this, SLOT(setQ1()));
    ::QObject::connect(_sectors[2], SIGNAL(changed()), this, SLOT(setQ2()));
    ::QObject::connect(_sectors[3], SIGNAL(changed()), this, SLOT(setQ3()));
  }
  else
    for(unsigned i=0; i<4; i++)
      ::QObject::connect(_sectors[i], SIGNAL(changed()), this, SIGNAL(changed()));
}

void Epix10ka2MMap::setQ0() { setQ(0); }
void Epix10ka2MMap::setQ1() { setQ(1); }
void Epix10ka2MMap::setQ2() { setQ(2); }
void Epix10ka2MMap::setQ3() { setQ(3); }

void Epix10ka2MMap::setQ(unsigned q) 
{
  for(unsigned i=0; i<4; i++)
    if (i==q)
      _sectors[i]->update(_sectors[i]->value());
    else
      _sectors[i]->update(0);
  emit changed();
}

void Epix10ka2MMap::update(uint64_t m)
{
  for(unsigned i=0; i<4; i++)
    _sectors[i]->update((m >> (i*16))&0xffff);
}

uint64_t Epix10ka2MMap::value() const
{
  uint64_t v = 0;
  for(unsigned i=0; i<4; i++)
    v |= uint64_t(_sectors[i]->value()&0xffff) << (16*i);
  return v;
}


  
