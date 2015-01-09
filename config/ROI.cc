#include "pdsapp/config/ROI.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QStackedWidget>
#include <QtCore/QString>

using namespace Pds_ConfigDb;

ROI::ROI(const char* label,
         const char* a,
         const char* b) : 
  Parameter(label),
  _sw(new QStackedWidget),
  _lo(0,0,0,0x1fff),
  _hi(0,0x1ff,0,0x1fff) 
{
  QString qa = QString("%1 %2 Range").arg(_label).arg(a);
  _sw->addWidget(new QLabel(qa));
  QString qb = QString("%1 %2 Range").arg(_label).arg(b);
  _sw->addWidget(new QLabel(qb));
}

QLayout* ROI::initialize(QWidget* p) {
  QHBoxLayout* h = new QHBoxLayout;
  h->addWidget(_sw);
  h->addLayout(_lo.initialize(p));
  h->addLayout(_hi.initialize(p));
  return h;
}

void ROI::update() {
  _lo.update();
  _hi.update();
  flush();
}

void ROI::flush() {
  if (_lo.value > _hi.value) {
    unsigned t=_lo.value;
    _lo.value=_hi.value;
    _hi.value=t;
  }
  _lo.flush();
  _hi.flush();
}

void ROI::enable(bool l) {
  _lo.enable(l);
  _hi.enable(l);
}
