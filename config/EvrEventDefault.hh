#ifndef Pds_EvrEventDefault_hh
#define Pds_EvrEventDefault_hh

#include <QtGui/QLabel>

#include "pds/config/EvrConfigType.hh"

#include <list>

namespace Pds_ConfigDb {
  class EvrEventDefault : public QLabel {
  public:
    EvrEventDefault();
    ~EvrEventDefault();
  public:   
    bool pull(const EventCodeType&);
    void push(EventCodeType*&) const;
    void flush();
    void clear();
  private:
    std::list<unsigned> _codes;
  };
};

#endif    
