#include "pdsapp/config/Epix10ka2MGainMap.hh"
#include "pdsapp/config/Epix10kaQuadGainMap.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"

#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

Epix10ka2MGainMap::Epix10ka2MGainMap(unsigned                   nq,
                                     ndarray<uint16_t,2>*       pixelArray,
                                     const Epix10kaASICdata*    asicConfig) :
  QWidget(0),
  _sectors(nq)
{
  static const Qt::Alignment horz[] = { ::Qt::AlignRight , ::Qt::AlignLeft };
  static const Qt::Alignment vert[] = { ::Qt::AlignBottom, ::Qt::AlignTop  };
  static const unsigned      row [] = { 0, 1, 1, 0 };
  static const unsigned      col [] = { 1, 1, 0, 0 };

  QGridLayout* ql = new QGridLayout;
  for(unsigned i=0; i<nq; i++) {
    ql->addWidget(_sectors[i] = new Epix10kaQuadGainMap(i,&pixelArray[4*i],
                                                        &asicConfig[16*i]),
                  row[i],col[i],horz[col[i]] | vert[row[i]]);
    ::QObject::connect(_sectors[i], SIGNAL(clicked(int)), this, SIGNAL(clicked(int)));
  }
  setLayout(ql);
}

Epix10ka2MGainMap::~Epix10ka2MGainMap()
{
}

void Epix10ka2MGainMap::update()
{
  for(unsigned i=0; i<_sectors.size(); i++)
    _sectors[i]->update();
}

