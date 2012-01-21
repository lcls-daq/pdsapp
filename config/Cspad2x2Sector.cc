#include "pdsapp/config/Cspad2x2Sector.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>

using namespace Pds_ConfigDb;

static const int _length  = 62;
static const int _width   = 30;
static const int xo[] = {
    _length+8,
    _length+_width+10,
    4, 4,
    _width+6, 4,
    _length+8, _length+8 };
static const int yo[] = {
    _length+8, _length+8,
    _length+8, _length+_width+10,
    4, 4,
    4, _width+6 };
static const int frame = 134;


Cspad2x2Sector::Cspad2x2Sector(QLineEdit& edit, unsigned q) : _edit(edit), _quad(0) {}

void Cspad2x2Sector::update(unsigned m)
{
  unsigned rm = m & 0xff;

  QPixmap* image = new QPixmap(frame, frame);

  QRgb bg = QPalette().color(QPalette::Window).rgb();
  image->fill(bg);

  QPainter painter(image);
  for(unsigned j=0; j<2; j++) {
    //	  QRgb fg = (rm   &(1<<j)) ? qRgb(255,255,255) : qRgb(0,0,0);
    QRgb fg = (rm   &(1<<j)) ? qRgb(0,255,255) : bg;
    painter.setBrush(QColor(fg));
    painter.drawRect(xo[j],yo[j], (j&2) ? _length : _width, (j&2) ? _width : _length);
  }

  setPixmap(*image);
//  setPixmap(*(new QPixmap(*image)));
}

void Cspad2x2Sector::mousePressEvent( QMouseEvent* e ) {
  int x = e->x();
  int y = e->y();

  bool ok;
  unsigned rm = _edit.text().toUInt(&ok,16);
  if (!ok) return;

  for(unsigned j=0; j<8; j++) {
    int dx = x-xo[j];
    int dy = y-yo[j];
    if (dx>0 && dy>0) {
      if (( (j&2) && (dx < _length) && (dy < _width) ) ||
          (!(j&2) && (dx < _width ) && (dy < _length))) {
        rm ^= 1<<(j+8*_quad);
        _edit.clear();
        _edit.insert(QString::number(rm,16));
        update(rm);
      }
    }
  }
}
