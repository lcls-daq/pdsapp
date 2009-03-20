#ifndef Pds_Parameter_hh
#define Pds_Parameter_hh

#include "pds/service/LinkedList.hh"

namespace Pds_ConfigDb {

  class Parameter : public Pds::LinkedList<Parameter> {
  public:
    Parameter(const char* label) : _label(label) {}
    ~Parameter() {}

    const char* label() const { return _label; }
  private:
    const char* _label;
  };

  template <class T>
  class Numeric : public Parameter {
  public:
    Numeric(const char* label, T val, T* rng=0) :
      Parameter(label),
      value(val) 
    { if (rng) memcpy(range,rng,2*sizeof(T)); }
    ~Numeric() {}
  public:
    T value;
    T range[2];
  };

  template <class T>
  class Enumerated : public Parameter {
  public:
    Enumerated(const char* label, T val, const char** strlist) :
      Parameter(label),
      value(val),
      labels(strlist)
    {}
    ~Enumerated() {}
  public:
    T value;
    const char** labels;
  };

};

#endif
