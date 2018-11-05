#include "pdsapp/config/Epix10ka2MMap.hh"
#include "pdsapp/config/EpixSector.hh"

#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

Epix10ka2MMap::Epix10ka2MMap(unsigned nq, unsigned g, bool e) : QWidget(0), _sectors(nq)
{
  static const Qt::Alignment horz[] = { ::Qt::AlignRight , ::Qt::AlignLeft };
  static const Qt::Alignment vert[] = { ::Qt::AlignBottom, ::Qt::AlignTop  };
  static const unsigned      row [] = { 0, 1, 1, 0 };
  static const unsigned      col [] = { 1, 1, 0, 0 };

  QGridLayout* ql = new QGridLayout;
  for(unsigned i=0; i<nq; i++)
    ql->addWidget(_sectors[i] = new EpixSector(i,g,e),row[i],col[i],horz[col[i]] | vert[row[i]]);
  setLayout(ql);

  if (e) {
    if (nq>0) ::QObject::connect(_sectors[0], SIGNAL(changed()), this, SLOT(setQ0()));
    if (nq>1) ::QObject::connect(_sectors[1], SIGNAL(changed()), this, SLOT(setQ1()));
    if (nq>2) ::QObject::connect(_sectors[2], SIGNAL(changed()), this, SLOT(setQ2()));
    if (nq>3) ::QObject::connect(_sectors[3], SIGNAL(changed()), this, SLOT(setQ3()));
  }
  else
    for(unsigned i=0; i<nq; i++)
      ::QObject::connect(_sectors[i], SIGNAL(changed()), this, SIGNAL(changed()));
}

Epix10ka2MMap::~Epix10ka2MMap()
{
}

void Epix10ka2MMap::setQ0() { setQ(0); }
void Epix10ka2MMap::setQ1() { setQ(1); }
void Epix10ka2MMap::setQ2() { setQ(2); }
void Epix10ka2MMap::setQ3() { setQ(3); }

void Epix10ka2MMap::setQ(unsigned q) 
{
  for(unsigned i=0; i<_sectors.size(); i++)
    if (i==q)
      _sectors[i]->update(_sectors[i]->value());
    else
      _sectors[i]->update(0);
  emit changed();
}

void Epix10ka2MMap::update(uint64_t m)
{
  for(unsigned i=0; i<_sectors.size(); i++)
    _sectors[i]->update((m >> (i*16))&0xffff);
}

uint64_t Epix10ka2MMap::value() const
{
  uint64_t v = 0;
  for(unsigned i=0; i<_sectors.size(); i++)
    v |= uint64_t(_sectors[i]->value()&0xffff) << (16*i);
  return v;
}


  
