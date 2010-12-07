#ifndef Pds_EvrGlbEventDesc_hh
#define Pds_EvrGlbEventDesc_hh

#include "pdsapp/config/EvrEventDesc.hh"

class QComboBox;

namespace Pds_ConfigDb {
  class EvrGlbEventDesc : public EvrEventDesc {
  public:
    EvrGlbEventDesc();
  public:   
    QWidget*       code_widget();
    unsigned       get_code() const;
    void set_code  (unsigned);
  private:
    QComboBox* _code;
  };
};

#endif    
