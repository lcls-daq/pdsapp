#include "pdsapp/config/CfgParameter.hh"

using namespace ConfigGui;

Parameter::Parameter(const char* label) :
  _label(label) 
{
}

Parameter::~Parameter() 
{
}

const char* Parameter::label() const 
{
  return _label; 
}


template class <T>
Numeric<T>::Numeric(const char* label, T val, T* rng=0) :
  Parameter(label),
  value(val) 
{
  if (rng) memcpy(range,rng,2*sizeof(T)); 
}

template class <T>
Numeric<T>::~Numeric() 
{
}


template class <T>
Enumerated<T>::Enumerated(const char* label, T val, const char** rng) :
  Parameter(label),
  value(val),
  range(rng)
{
}

template class <T>
Enumerated<T>::~Enumerated() 
{
}
