#ifndef Pds_EvrSeqEventDesc_hh
#define Pds_EvrSeqEventDesc_hh

#include "pdsapp/config/EvrEventDesc.hh"
#include "pdsapp/config/Parameters.hh"

class QLabel;

namespace Pds_ConfigDb {
  class EvrSeqEventDesc : public EvrEventDesc {
  public:
    EvrSeqEventDesc();
  public:   
    QWidget* code_widget();
    unsigned get_code() const;
    void     set_code  (unsigned);
  private:
    NumericInt<unsigned> _code;
    QWidget*             _widget;
  };
};

#endif    
