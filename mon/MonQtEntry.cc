#include <stdio.h>

#include "MonQtEntry.hh"

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QIntValidator>
#include <QtGui/QHBoxLayout>

using namespace Pds;

// MonQtEntry

MonQtEntry::MonQtEntry(const char* title, QWidget* p) :
  QWidget(p),
  _line  (new QLineEdit(this))
{
  _line->setMaximumWidth(60);
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(new QLabel(title));
  layout->addWidget(_line);
  setLayout(layout);
  layout->setContentsMargins(0,0,0,0);
}

void MonQtEntry::setEntry(const char* s)
{
  _line->setText(QString(s));
}

// MonQtFloatEntry

MonQtFloatEntry::MonQtFloatEntry(const char* title, float v, QWidget* p) :
  MonQtEntry(title, p)
{
  setEntry(v);
  _line->setValidator(new QDoubleValidator(_line));
}

void MonQtFloatEntry::setEntry(float e)
{
  char buff[32];
  sprintf(buff,"%g",e);
  MonQtEntry::setEntry(buff);
}

float MonQtFloatEntry::entry() const
{
  return _line->text().toFloat();
}

// MonQtIntEntry

MonQtIntEntry::MonQtIntEntry(const char* title, int v, QWidget* p) :
  MonQtEntry(title, p)
{
  setEntry(v);
  _line->setValidator(new QIntValidator(_line));
}

void MonQtIntEntry::setEntry(int e)
{
  char buff[32];
  sprintf(buff,"%d",e);
  MonQtEntry::setEntry(buff);
}

int MonQtIntEntry::entry() const
{
  return _line->text().toInt();
}
