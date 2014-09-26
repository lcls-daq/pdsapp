#include "pdsapp/config/QrLabel.hh"

#include <QtGui/QPainter>

using namespace Pds_ConfigDb;

QrLabel::QrLabel() : QLabel() {}

QrLabel::QrLabel(const QString& s) : QLabel() { setText(s); }

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
  QRect r(1,0,rect().height(),rect().width()-1);
  painter.drawText(r,0, text());
  painter.drawLine(1,0,1,rect().width()-1);
}

void QrLabel::setText(const QString& s)
{
  QString t(s);
  QLabel::setText(t.replace(',','\n'));
  resize(sizeHint());
}
