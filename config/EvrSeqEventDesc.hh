#ifndef Pds_EvrSeqEventDesc_hh
#define Pds_EvrSeqEventDesc_hh

#include "pdsapp/config/EvrEventDesc.hh"

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
    QLabel* _code;
  };
};

#endif    
