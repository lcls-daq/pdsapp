#ifndef PdsConfigDb_Epix10ka2MGainMap_hh
#define PdsConfigDb_Epix10ka2MGainMap_hh

#include <QtGui/QWidget>
#include <ndarray/ndarray.h>
#include <stdint.h>
#include <vector>

namespace Pds_ConfigDb
{
  class Epix10kaASICdata;
  class Epix10kaQuadGainMap;

  class Epix10ka2MGainMap : public QWidget {
    Q_OBJECT
  public:
    Epix10ka2MGainMap(unsigned, ndarray<uint16_t,2>*, const Epix10kaASICdata*);
    ~Epix10ka2MGainMap();
  public slots:
    void update();
  signals:
    void clicked(int);
  private:
    std::vector<Epix10kaQuadGainMap*> _sectors;
  };
};

#endif
