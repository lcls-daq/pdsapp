#include "pdsapp/config/QrLabel.hh"

#include <QtGui/QPainter>

using namespace Pds_ConfigDb;

QrLabel::QrLabel() : QLabel() { setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); }

QrLabel::QrLabel(const QString& s) : QLabel(s) { setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); }

QSize QrLabel::sizeHint() const {
  QSize h(20, 87);
  return h;
}

void QrLabel::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.rotate(-90);
  painter.translate(-rect().height(),0);
  QRect r(0,0,rect().height(),rect().width());
  painter.drawText(r,0, text());
}

void QrLabel::setText(const QString& s)
{
  QLabel::setText(s);
  resize(sizeHint());
}

