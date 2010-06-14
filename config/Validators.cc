#include "pdsapp/config/Validators.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QLineEdit>

using namespace Pds_ConfigDb;

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


HexValidator::HexValidator(Parameter& p, QLineEdit& l,
			   unsigned rlo, unsigned rhi) :
  QValidator(&l),
  _p        (p),
  _rlo      (rlo),
  _rhi      (rhi)
{
  connect(&l, SIGNAL(editingFinished()), this, SLOT(validChange()));
}

HexValidator::~HexValidator() {}

void HexValidator::fixup(QString& str) const
{
  _p.flush();
}

void HexValidator::validChange()
{
  _p.update();
}

QValidator::State HexValidator::validate(QString& input,
					 int& pos) const
{
  if (input.size()==0) return Intermediate;
  bool ok;
  unsigned v = input.toInt(&ok, 16);
  if (!ok) return Invalid;
  if (v < _rlo || v > _rhi) return Invalid;
  return Acceptable;
}


DoubleValidator::DoubleValidator(Parameter& p, QLineEdit& l,
				 double rlo, double rhi) :
  QDoubleValidator(&l),
  _p              (p)
{
  setRange(rlo,rhi);
  setDecimals(10);
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
  _p.flush();
}

