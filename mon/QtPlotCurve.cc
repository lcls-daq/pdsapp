#include "pdsapp/mon/QtPlotCurve.hh"

using namespace Pds;

QtPlotCurve::QtPlotCurve(const char* name) : QwtPlotCurve(name) 
{
  connect(this, SIGNAL(setRaw()), this, SLOT(rawSet()));
}

QtPlotCurve::~QtPlotCurve() {}

void QtPlotCurve::setRawData(double* x, double* y, unsigned n)
{
  _rawx = x;
  _rawy = y;
  _rawn = n;
  emit setRaw();
}

void QtPlotCurve::rawSet()
{
  QwtPlotCurve::setRawData(_rawx,_rawy,_rawn);
}
