#include "pdsapp/config/EvrGlbEventDesc_V6.hh"

#include <QtGui/QComboBox>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>


static const char* code_str[] = { "40 [120 Hz]",
                                  "41 [ 60 Hz]",
                                  "42 [ 30 Hz]",
                                  "43 [ 10 Hz]",
                                  "44 [  5 Hz]",
                                  "45 [  1 Hz]",
                                  "46 [0.5 Hz]",
                                  "140 [ Every Beam]",
                                  "141 [ 60 Hz Beam]",
                                  "142 [ 30 Hz Beam]",
                                  "143 [ 10 Hz Beam]",
                                  "144 [  5 Hz Beam]",
                                  "145 [  1 Hz Beam]",
                                  "146 [0.5 Hz Beam]",
                                  "150 [Burst Mode]",
                                  "162 [BYKIK]",
                                  NULL };
                                  
using namespace Pds_ConfigDb::EvrConfig_V6;

EvrGlbEventDesc::EvrGlbEventDesc() {}

QWidget* EvrGlbEventDesc::code_widget() {
  _code = new QComboBox;
  for(unsigned i=0; code_str[i]!=NULL; i++)
    _code->addItem(code_str[i]);
  _code->setCurrentIndex(0);
  return _code;
}

unsigned EvrGlbEventDesc::get_code() const
{
  return _code->currentText().split(' ')[0].toUInt();
}

void EvrGlbEventDesc::set_code(unsigned n)
{
  for(unsigned i=0; code_str[i]!=NULL; i++)
    if (QString(code_str[i]).split(' ')[0].toUInt()==n)
      _code->setCurrentIndex(i);
}

bool EvrGlbEventDesc::global_code(unsigned e)
{
  return (e>=40 && e<=46) || (e>=140 && e<=146) || (e==150) || (e==162);
}
