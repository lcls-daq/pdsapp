#ifndef Pds_EvrSeqEventDesc_V6_hh
#define Pds_EvrSeqEventDesc_V6_hh

#include "pdsapp/config/EvrEventDesc_V6.hh"

class QLabel;

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {

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
};

#endif    
