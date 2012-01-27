#include "pdsapp/config/Cspad2x2Sector.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>

using namespace Pds_ConfigDb;

static const int _length  = 185; //62;
static const int _width   = 92; //30;
static const int xo[] = { 8,  _width + 10 };
static const int yo[] = { 8,  8           };


Cspad2x2Sector::Cspad2x2Sector(QLineEdit& edit, unsigned q) : _edit(edit) {}

void Cspad2x2Sector::update(unsigned m)
{
  unsigned rm = m & 0xff;

  QPixmap* pixmap = new QPixmap(_width*2+20, _length+16);

  QRgb bg = QPalette().color(QPalette::Window).rgb();
  pixmap->fill(bg);

  QPainter painter(pixmap);
  for(unsigned j=0; j<2; j++) {
    QRgb fg = (rm & (1<<j)) ? qRgb(0,255,128) : bg;
    painter.setBrush(QColor(fg));
    painter.drawRect(xo[j],yo[j],  _width,  _length);
  }

  setPixmap(*pixmap);
}

void Cspad2x2Sector::mousePressEvent( QMouseEvent* e ) {
  int x = e->x();
  int y = e->y();

  bool ok;
  unsigned rm = _edit.text().toUInt(&ok,16);
  if (!ok) return;

  for(unsigned j=0; j<2; j++) {
    int dx = x-xo[j];
    int dy = y-yo[j];
    if (dx>0 && dy>0) {
      if ((dx < _width ) && (dy < _length)) {
        rm ^= 1<<(j);
        _edit.clear();
        _edit.insert(QString::number(rm,16));
        update(rm);
      }
    }
  }
}
