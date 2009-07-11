#include "pdsapp/config/Parameters.icc"

using namespace Pds_ConfigDb;

template class Enumerated<Enums::Bool>;
template class Enumerated<Enums::Polarity>;
template class Enumerated<Enums::Enabled>;
template class NumericInt<unsigned short>;
template class NumericInt<unsigned>;
template class NumericInt<int>;
template class NumericFloat<double>;
