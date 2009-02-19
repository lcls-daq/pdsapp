#include "MonRasterData.hh"

using namespace Pds;

MonRasterData::MonRasterData(unsigned nx, unsigned ny,
			     double xlo, double xhi,
			     double ylo, double yhi,
			     float& zlo, float& zhi) :
  QwtRasterData(QwtDoubleRect(xlo,ylo,xhi,yhi)),
  _z(new float[nx*ny]),
  _zlo(zlo),
  _zhi(zhi),
  _nx(nx),
  _ny(ny),
  _xlo(xlo),
  _ylo(ylo),
  _idx(double(nx)/(xhi-xlo)),
  _idy(double(ny)/(yhi-ylo))
{
}

MonRasterData::MonRasterData(const MonRasterData& d) :
  QwtRasterData(d),
  _z  (new float[d._nx*d._ny]),
  _zlo(d._zlo),
  _zhi(d._zhi),
  _nx (d._nx),
  _ny (d._ny),
  _xlo(d._xlo),
  _ylo(d._ylo),
  _idx(d._idx),
  _idy(d._idy)
{
  memcpy(_z, d._z, _nx*_ny*sizeof(float));
}

MonRasterData::~MonRasterData()
{
  delete[] _z;
}

QwtRasterData* MonRasterData::copy() const
{
  return new MonRasterData(*this);
}

QwtDoubleInterval MonRasterData::range() const
{
  double zlo(_zlo);
  double zhi(_zhi);
  return QwtDoubleInterval(zlo, zhi);
  //  return QwtDoubleInterval(0,1);
}

//
//  Qwt scans the raster from top left to bottom right.
//  The consequence is that bin contents are drawn based
//  upon the value returned for the top-left corner of 
//  that bin.
//
double MonRasterData::value(double x,double y) const
{
  int ix = _xpixel(x);
  int iy = _ypixel(y);
  int idx = iy > 1 ? ix+(iy-1)*_nx : ix;
  return _z[idx];
}

//
//  Report the number of bins given with the rectangle
//
QSize MonRasterData::rasterHint(const QwtDoubleRect& r) const
{
  QSize v(_xpixel(r.right ())-_xpixel(r.left()),
	  _ypixel(r.bottom())-_ypixel(r.top()));
  return v;
}

