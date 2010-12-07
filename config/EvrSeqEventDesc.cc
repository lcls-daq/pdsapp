#include "pdsapp/config/EvrSeqEventDesc.hh"

#include <QtGui/QLabel>

using namespace Pds_ConfigDb;

EvrSeqEventDesc::EvrSeqEventDesc() {}

QWidget* EvrSeqEventDesc::code_widget()
{
  _code = new QLabel("0");
  _code->setMaximumWidth(40);
  return _code;
}

unsigned EvrSeqEventDesc::get_code() const
{
  return _code->text().toInt();
}

void EvrSeqEventDesc::set_code(unsigned n)
{
  _code->setText(QString::number(n,10));
}

