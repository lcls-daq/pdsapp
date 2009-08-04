#include "pdsapp/config/BitCount.hh"

#include "pdsapp/config/ParameterSet.hh"
#include <QtGui/QLineEdit>

using namespace Pds_ConfigDb;

BitCount::BitCount(NumericInt<uint32_t>& mask) : _mask(mask) {}

BitCount::~BitCount() {}

bool BitCount::connect(ParameterSet& set)
{
  return QObject::connect(_mask._input, SIGNAL(editingFinished()),
			  &set, SLOT(membersChanged()));
}

unsigned BitCount::count()
{
  unsigned n=0;
  for (unsigned i=0;i<32;i++) if (_mask.value&(1<<i)) n++;
  return n;
}
