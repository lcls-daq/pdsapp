#include "pdsapp/config/EvrSeqEventDesc.hh"

#include <QtGui/QLabel>

using namespace Pds_ConfigDb;

EvrSeqEventDesc::EvrSeqEventDesc() :
  _code(NULL, 0, 0, 255),
  _widget(new QWidget)
{
  _widget->setLayout(_code.initialize(0));
  _widget->setMaximumWidth(40);
}

QWidget* EvrSeqEventDesc::code_widget()
{
  return _widget;
}

unsigned EvrSeqEventDesc::get_code() const
{
  const_cast<EvrSeqEventDesc*>(this)->_code.update();
  return _code.value;
}

void EvrSeqEventDesc::set_code(unsigned n)
{
  _code.value = n;
  _code.flush();
}

