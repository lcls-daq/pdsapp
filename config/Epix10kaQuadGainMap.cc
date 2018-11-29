#include "pdsapp/config/Epix10kaQuadGainMap.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"

#include <QtGui/QColor>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QStaticText>

using namespace Pds_ConfigDb;

static const int _height  = 44;
static const int _width   = 48;
static const int asic_xo[] = { 4, 4, _width+5, _width+5 };
static const int asic_yo[] = { 11, _height+12, _height+12, 11 }; 
static const int elem_xo[] = { 0, 2*_width+6, 0, 2*_width+6 };
static const int elem_yo[] = { 0, 0, 2*_height+6, 2*_height+6 };

static const int framew = 218;
static const int frameh = 192;

static const unsigned pixelConfigShape[3] = {4, 352, 384};

static const unsigned _r[] = {    0,    0, 0xff, 0x7f, 0x7f };
static const unsigned _g[] = {    0, 0xff,    0,    0, 0x7f };
static const unsigned _b[] = { 0xff,    0,    0, 0x7f,    0 };


unsigned Epix10kaQuadGainMap::rgb(GainMode gm)
{
  return (0xff<<24) | (_r[gm]<<16) | (_g[gm]<<8) | (_b[gm]<<0);
}

Epix10kaQuadGainMap::Epix10kaQuadGainMap(unsigned                   quad,
                                         ndarray<uint16_t,2>*       pixelConfig,
                                         const Epix10kaASICdata*    asicConfig) :
  QLabel      (0),
  _quad       (quad),
  _pixelConfig(pixelConfig),
  _asicConfig (asicConfig)
{
}

Epix10kaQuadGainMap::~Epix10kaQuadGainMap()
{
}

void Epix10kaQuadGainMap::update()
{
  QPixmap* image = new QPixmap(framew, frameh);

  QRgb bg = QPalette().color(QPalette::Window).rgb();
  image->fill(bg);

  QPainter painter(image);
  for(unsigned j=0; j<16; j++) {
    int ie = j/4;
    int ia = j%4;
    int xo = elem_xo[ie] + asic_xo[ia];
    int yo = elem_yo[ie] + asic_yo[ia];
    unsigned trbit = _asicConfig[j]._reg[Epix10kaASIC_ConfigShadow::trbit]->value;
    // loop over pixels
    ndarray<uint16_t,2> pa = _pixelConfig[ie];
    const unsigned* sh = pa.shape();
    unsigned xoff=0, yoff=0;

    switch(ia) {
    case 0: yoff = sh[0]  -1; xoff = sh[1]  -1; break;
    case 1: yoff = sh[0]/2-1; xoff = sh[1]  -1; break;
    case 2: yoff = sh[0]/2-1; xoff = sh[1]/2-1; break;
    case 3: yoff = sh[0]  -1; xoff = sh[1]/2-1; break;
    }
    for(unsigned y=0; y<_height; y++) {
      for(unsigned x=0; x<_width; x++) {
        unsigned r=0,g=0,b=0;
        for(unsigned iy=0; iy<4; iy++) {
          for(unsigned ix=0; ix<4; ix++) {
            unsigned mapv  = pa[yoff-(y*4+iy)][xoff-(x*4+ix)];
            unsigned gm = 0;
            if (mapv==12)     gm = trbit ? 0 : 1;
            else {
              if (mapv==8) gm = 2;
              else {
                if (trbit)   gm = 3;
                else              gm = 4;
              }
            }
            r += _r[gm];
            g += _g[gm];
            b += _b[gm];
          }
        }
        r >>= 4; g >>= 4; b >>= 4;
        //  calculate pixel color
        QRgb fg = (0xff<<24) | (r<<16) | (g<<8) | (b<<0);
        painter.setPen(QColor(fg));
        painter.drawPoint(xo+x, yo+y);
      }
    }

    painter.setPen(QColor(QRgb(0)));
    QString label = QString("%1").arg(j);
    painter.drawStaticText(xo+_width/2-4,yo+_height/2-6,QStaticText(label));
  }
  QString label = QString("Quad%1").arg(_quad);
  painter.drawStaticText(2*_width-10,0,QStaticText(label));

  QTransform transform(QTransform().rotate(90*_quad));
  setPixmap(image->transformed(transform));
}

void Epix10kaQuadGainMap::mousePressEvent( QMouseEvent* e ) {
  int x,y;
  switch(_quad) {
  case 0: x=      e->x(); y=      e->y(); break;
  case 1: x=      e->y(); y=frameh-e->x(); break;
  case 2: x=framew-e->x(); y=frameh-e->y(); break;
  case 3: x=framew-e->y(); y=      e->x(); break;
  default: return;
  }

  for(unsigned j=0; j<16; j++) {
    int xo = elem_xo[j/4] + asic_xo[j%4];
    int yo = elem_yo[j/4] + asic_yo[j%4];
    int dx = x-xo;
    int dy = y-yo;
    if (dx>0 && dy>0) {
      if ((dx < _width) && (dy < _height)) {
        emit clicked(16*_quad+j);
      }
    }
  }
}

