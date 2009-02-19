#ifndef Pds_MonRasterData_hh
#define Pds_MonRasterData_hh

#include "qwt_raster_data.h"

namespace Pds {

  class MonRasterData : public QwtRasterData {
  public:
    MonRasterData(unsigned nx,unsigned ny,
		  double xlo, double xhi,
		  double ylo, double yhi,
		  float& zlo, float& zhi);
    MonRasterData(const MonRasterData&);
    ~MonRasterData();

    QwtRasterData*    copy () const;
    QwtDoubleInterval range() const;
    double            value(double,double) const;

    virtual QSize     rasterHint(const QwtDoubleRect&) const;

    float*            data() { return _z; }
  private:
    int               _xpixel(double) const;
    int               _ypixel(double) const;

    float*   _z;
    float&   _zlo;
    float&   _zhi;
    unsigned _nx , _ny ;
    double   _xlo, _ylo;
    double   _idx, _idy;
  };

  inline int MonRasterData::_xpixel(double x) const
  {
    return int((x-_xlo)*_idx);
  }

  inline int MonRasterData::_ypixel(double y) const
  {
    return int((y-_ylo)*_idy);
  }

};
#endif
