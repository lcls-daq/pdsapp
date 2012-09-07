#ifndef Pds_EvrGlbEventDesc_V6_hh
#define Pds_EvrGlbEventDesc_V6_hh

#include "pdsapp/config/EvrEventDesc_V6.hh"

class QComboBox;

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {
    class EvrGlbEventDesc : public EvrEventDesc {
    public:
      EvrGlbEventDesc();
    public:   
      QWidget*       code_widget();
      unsigned       get_code() const;
      void set_code  (unsigned);
    public:
      static bool    global_code(unsigned);
    private:
      QComboBox* _code;
    };
  };
};

#endif    
