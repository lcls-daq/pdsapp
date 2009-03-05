#include "ConfigTC_Validators.hh"
#include "ConfigTC_Parameters.hh"

#include <QtGui/QLineEdit>

using namespace ConfigGui;

IntValidator::IntValidator(Parameter& p, QLineEdit& l,
			   int rlo, int rhi) :
  QIntValidator(&l),
  _p           (p)
{
  setRange(rlo,rhi);
  connect(&l, SIGNAL(editingFinished()), this, SLOT(validChange()));
}

IntValidator::~IntValidator() {}

void IntValidator::fixup(QString& str) const
{
  _p.flush();
}

void IntValidator::validChange()
{
  _p.update();
}


DoubleValidator::DoubleValidator(Parameter& p, QLineEdit& l,
				 double rlo, double rhi) :
  QDoubleValidator(&l),
  _p              (p)
{
  setRange(rlo,rhi);
  connect(&l, SIGNAL(editingFinished()), this, SLOT(validChange()));
}

DoubleValidator::~DoubleValidator() {}

void DoubleValidator::fixup(QString& str) const
{
  _p.flush();
}

void DoubleValidator::validChange()
{
  _p.update();
}

