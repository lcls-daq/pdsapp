#ifndef PdsConfigDb_Epix10kaQuadGainMap_hh
#define PdsConfigDb_Epix10kaQuadGainMap_hh

#include "pds/config/EpixConfigType.hh"
#include <ndarray/ndarray.h>

#include <QtGui/QLabel>
#include <stdint.h>

namespace Pds_ConfigDb
{
  class Epix10kaASICdata;

  class Epix10kaQuadGainMap : public QLabel {
    Q_OBJECT
  public:
    Epix10kaQuadGainMap(unsigned quad, ndarray<uint16_t,2>*, const Epix10kaASICdata*);
    ~Epix10kaQuadGainMap();
  public:
    enum GainMode { _HIGH_GAIN, _MEDIUM_GAIN, _LOW_GAIN, _AUTO_HL_GAIN, _AUTO_ML_GAIN };
    static uint32_t rgb(GainMode);                                                                public slots:
    void update();
  signals:
    void clicked(int);
  protected:
    void mousePressEvent( QMouseEvent* e );
  private:
    unsigned                   _quad;
    ndarray<uint16_t,2>*       _pixelConfig;
    const Epix10kaASICdata*    _asicConfig;
  };
};

#endif
