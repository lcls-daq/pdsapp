#include "MonQtImage.hh"
#include "MonQtTH1F.hh"
#include "MonQtImageDisplay.hh"

#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonDescImage.hh"

#include <QtGui/QImage>

#include <math.h>

using namespace Pds;

static const double DefaultZMin =     0.;
static const double DefaultZMax = 65535.;

static int shift_scale(unsigned scale, unsigned mask);

MonQtImage::MonQtImage(const char* name, 
		       const MonDescImage& desc) : 
  MonQtBase(Image, desc, name),
  _zmin(0),
  _zmax(256),
  _shift(-1),
  _display(0)
{
  params(desc);

  _contents = new unsigned[desc.nbinsx()*desc.nbinsy()];
  _qimage   = new QImage(desc.nbinsx(),desc.nbinsy(), QImage::Format_Indexed8);
  _qimage->fill(128);

  _color_table = new QVector<QRgb>(256);
  for (int i = 0; i < 256; i++)
    _color_table->insert(i, qRgb(i,i,i));
  _qimage->setColorTable(*_color_table);

  settings(MonQtBase::Z, _zmin, _zmax, true, false);
}

MonQtImage::~MonQtImage() 
{
  delete _qimage;
  delete _color_table;
  delete[] _contents;
}

void MonQtImage::setto(const MonEntryImage& entry) 
{
  last(entry.last());

  const MonDescImage& d(reinterpret_cast<const MonDescImage&>(*_desc));
  unsigned nx = d.nbinsx();
  unsigned ny = d.nbinsy();
  const unsigned* src = entry.contents();
  const unsigned* end = src + nx*ny;
  unsigned* buf = _contents;
  unsigned scale = 0;
  while( src < end )
    scale |= *buf++ = *src++;

  if (!_display) return;

  int shift = get_shift(scale);

  unsigned char* dst = _qimage->bits();
  src = entry.contents();
  while( src < end )
    *dst++ = (*src++)>>shift;

  _display->display(*_qimage);
}

void MonQtImage::setto(const MonEntryImage& curr, 
		       const MonEntryImage& prev) 
{
  last(curr.last());

  const MonDescImage& d(reinterpret_cast<const MonDescImage&>(*_desc));
  unsigned nx = d.nbinsx();
  unsigned ny = d.nbinsy();
  const unsigned* srcc = curr.contents();
  const unsigned* srcp = prev.contents();
  unsigned* buf = _contents;
  unsigned* const end = buf + nx*ny;
  unsigned scale = 0;
  while( buf < end ) {
    scale |= *buf++ = (*srcc > *srcp) ? (*srcc - *srcp) : 0;
    srcc++;
    srcp++;
  }

  if (!_display) return;

  int shift = get_shift(scale);

  unsigned char* dst = _qimage->bits();
  buf = _contents;
  while( buf < end )
    *dst++ = (*buf++)>>shift;

  _display->display(*_qimage);
}

void MonQtImage::projectx(MonQtTH1F* h) 
{
  h->reset();
  h->last(last());
  const MonDescImage& d(reinterpret_cast<const MonDescImage&>(*_desc));
  int nx = d.nbinsx();
  int ny = d.nbinsy();
  const unsigned* z = _contents;
  for (int ybin=0; ybin<ny; ybin++) {
    for (int xbin=0; xbin<nx; xbin++) {
      h->addcontent(xbin, float(*z++));
    }
  }
}

void MonQtImage::projecty(MonQtTH1F* h) 
{
  h->reset();
  h->last(last());
  const MonDescImage& d(reinterpret_cast<const MonDescImage&>(*_desc));
  int nx = d.nbinsx();
  int ny = d.nbinsy();
  const unsigned* z = _contents;
  for (int ybin=0; ybin<ny; ybin++) {
    for (int xbin=0; xbin<nx; xbin++) {
      h->addcontent(ybin, float(*z++));
    }
  }
}

float MonQtImage::min(Axis ax) const 
{
  return (ax == X) ? _xmin : (ax==Y) ? _ymin : _zmin;
}

float MonQtImage::max(Axis ax) const 
{
  return (ax == X) ? _xmax : (ax==Y) ? _ymax : _zmax;
}

void MonQtImage::dump(FILE* f) const
{
  const MonDescImage& d(reinterpret_cast<const MonDescImage&>(*_desc));
  unsigned nx = d.nbinsx();
  unsigned ny = d.nbinsy();
  const unsigned* z = _contents;
  for(unsigned j=0; j<ny; j++) {
    for(unsigned k=0; k<nx; k++)
      fprintf(f,"%d ",*z++);
    fprintf(f,"\n");
  }
}

void MonQtImage::color(int color)
{
}

int  MonQtImage::color() const
{
  return 0;
}

void MonQtImage::attach(QwtPlot* plot)
{
}

void MonQtImage::attach(MonQtImageDisplay* display)
{
  _shift = -1;  // force redraw of the axis
  _display = display;
  if (display) display->resize(_qimage->size());
}

void MonQtImage::settings(MonQtBase::Axis ax, float vmin, float vmax,
			  bool autorng, bool islog)
{
  switch(ax) {
//   case MonQtBase::X: _xmin=vmin; _xmax=vmax; break;
//   case MonQtBase::Y: _ymin=vmin; _ymax=vmax; break;
  case MonQtBase::Z: _zmin=vmin; _zmax=vmax; break;
  default: break;
  }
  MonQtBase::settings(ax,vmin,vmax,autorng,islog);

  _shift = -1;  // force compute_color_map at next update
}

int MonQtImage::get_shift(int scale)
{
  if (!isautorng(MonQtBase::Z)) {  //  The scale is fixed
    if (_shift<0) {  // Compute the scale and set the color map
      double zmin(_zmin);
      double zmax(_zmax);
      //  Validate [_zmin,_zmax]
      if (zmin <    0) zmin = 0;
      if (zmax < zmin) zmax = zmin+1;
      //
      //  Data is shifted into the 0..255 range,
      //  but the color map fits within a subrange
      //
      _shift = shift_scale(unsigned(_zmax),0xff);
      int imin = unsigned(zmin) >> _shift;
      int imax = unsigned(zmax) >> _shift;
      if (islog(MonQtBase::Z)) {
	double z0 = 1.*(1<<_shift);
	if (zmin < z0) zmin = z0;
	const double logsc0 = 255. / log(imax-imin+1);
	for (int i = 0; i < 256; i++) {
	  int y;
	  if      (i < imin) y = 0;
	  else if (i < imax) y = int(logsc0*log(double(i-imin+1)));
	  else               y = 255;
	  _color_table->insert(i, qRgb(y,y,y));
	}
      }
      else {
	for (int i = 0; i < 256; i++) {
	  int y;
	  if      (i < imin) y = 0;
	  else if (i < imax) y = int( double(i - imin)*256/double(imax - imin+1) );
	  else               y = 255;
	  _color_table->insert(i, qRgb(y,y,y));
	}
      }
      _display->axis(*_color_table, imin, imax, zmin, zmax);
      _qimage->setColorTable(*_color_table);
    }
  }
  else {  // The scale is set by the data
    int shift = shift_scale(scale,0xff);
    if (shift != _shift) {  // Set the color map
      _shift = shift;
      double zmin = 0;
      double zmax = 256.*(1<<_shift);
      //
      //  All data is shifted into the 0..255 range,
      //  and the color map spans the full range.
      //  The axis labels adjust accordingly.
      //
      if (islog  (MonQtBase::Z)) {
	zmin = 1.*(1<<_shift);
	const double logsc0 = 255. / log(256.);
	for (int i = 0; i < 256; i++) {
	  int y = int(logsc0*log(double(i+1)));
	  _color_table->insert(i, qRgb(y,y,y));
	}
      }
      else {
	for (int i = 0; i < 256; i++)
	  _color_table->insert(i, qRgb(i,i,i));
      }
      _display->axis(*_color_table, 0, 256, zmin, zmax);
      _qimage->setColorTable(*_color_table);
    }
  }
  return _shift;
}

int shift_scale(unsigned scale, unsigned mask)
{
  int shift = 0;
  while(scale & ~mask) {
    shift++;
    scale>>=1;
  }
  return shift;
}
