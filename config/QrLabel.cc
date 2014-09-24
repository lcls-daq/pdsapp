#include "pdsapp/config/QrLabel.hh"

#include <QtGui/QPainter>

using namespace Pds_ConfigDb;

QrLabel::QrLabel() : QLabel() {}

QrLabel::QrLabel(const QString& s) : QLabel(s) {}

QSize QrLabel::minimumSizeHint() const {
  QSize s = QLabel::minimumSizeHint();
  return QSize(s.height(), s.width());
}

QSize QrLabel::sizeHint() const {
  QSize s = QLabel::sizeHint();
  return QSize(s.height(), s.width());
}

void QrLabel::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setPen(Qt::black);
  painter.setBrush(Qt::Dense1Pattern);
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
