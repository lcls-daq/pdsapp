#include "pdsapp/config/EpixSector.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QStaticText>

using namespace Pds_ConfigDb;

static const int _height  = 31;
static const int _width   = 30;
static const int asic_xo[] = { 4, 4, _width+5, _width+5 };
static const int asic_yo[] = { 11, _height+12, _height+12, 11 }; 
static const int elem_xo[] = { 0, 2*_width+6, 0, 2*_width+6 };
static const int elem_yo[] = { 0, 0, 2*_height+6, 2*_height+6 };

static const int frame = 146;


EpixSector::EpixSector(unsigned q, 
                       unsigned grouping,
                       bool     exclusive) : 
  QLabel(0), 
  _quad(q), 
  _value(0xffff), 
  _grouping(grouping),
  _exclusive(exclusive) 
{
}

void EpixSector::update(unsigned m)
{
  unsigned rm = 0, gm=0;
  // for(unsigned i=0; i<_grouping; i++)
  //   gm |= ((1<<_grouping)-1)<<(4*i);
  gm = (1<<(_grouping*_grouping))-1;

  for(unsigned i=0; i<16/(_grouping*_grouping); i++) {
    if (m & gm)
      rm |= gm;
    gm <<= (_grouping*_grouping);
  }

  _value = rm = rm & 0xffff;

  QPixmap* image = new QPixmap(140, 146);

  QRgb bg = QPalette().color(QPalette::Window).rgb();
  image->fill(bg);

  QPainter painter(image);
  for(unsigned j=0; j<16; j++) {
    QRgb fg = (rm   &(1<<j)) ? qRgb(0,255,0) : bg;
    painter.setBrush(QColor(fg));
    int xo = elem_xo[j/4] + asic_xo[j%4];
    int yo = elem_yo[j/4] + asic_yo[j%4];
    painter.drawRect(xo, yo, _width, _height);
    painter.setBrush(QColor(qRgb(0,0,0)));
    QString label = QString("%1").arg(j);
    painter.drawStaticText(xo+_width/2-4,yo+_height/2-6,QStaticText(label));
  }
  QString label = QString("Quad%1").arg(_quad);
  painter.drawStaticText(2*_width-10,0,QStaticText(label));

  QTransform transform(QTransform().rotate(90*_quad));
  setPixmap(image->transformed(transform));
}

unsigned EpixSector::value() const
{
  return _value;
}

void EpixSector::mousePressEvent( QMouseEvent* e ) {
  int x,y;
  switch(_quad) {
  case 0: x=      e->x(); y=      e->y(); break;
  case 1: x=      e->y(); y=frame-e->x(); break;
  case 2: x=frame-e->x(); y=frame-e->y(); break;
  case 3: x=frame-e->y(); y=      e->x(); break;
  default: return;
  }

  //  bool ok;
  //  uint64_t rm = _edit.text().toUInt(&ok,16);
  //  if (!ok) return;
  unsigned rm = _exclusive? 0:_value;

  for(unsigned j=0; j<16; j++) {
    int xo = elem_xo[j/4] + asic_xo[j%4];
    int yo = elem_yo[j/4] + asic_yo[j%4];
    int dx = x-xo;
    int dy = y-yo;
    if (dx>0 && dy>0) {
      if ((dx < _width) && (dy < _height)) {
	rm ^= 1U<<j;
	update(rm);
        emit changed();
      }
    }
  }
}
