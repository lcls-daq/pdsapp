#include "ConfigTC_Parameters.icc"

using namespace ConfigGui;

template class Enumerated<Enums::Bool>;
template class Enumerated<Enums::Polarity>;
template class Enumerated<Enums::Enabled>;
template class NumericInt<unsigned short>;
template class NumericInt<unsigned>;
template class NumericInt<int>;
