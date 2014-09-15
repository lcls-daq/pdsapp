#ifndef PdsConfigDb_Epix10kConfigP_hh
#define PdsConfigDb_Epix10kConfigP_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsapp/config/Epix10kCopyAsicDialog.hh"

class QButtonGroup;

namespace Pds_ConfigDb {
  class Epix10kSimpleCount;
  class Epix10kASICdata;

  class Epix10kConfigP : public Parameter {
  public:
    Epix10kConfigP();
    ~Epix10kConfigP();
    enum { Off, On };
  public:
    QLayout* initialize(QWidget*);
    void update();
    void flush ();
    void enable(bool v);
  public:
    int pull(void* from);
    int push(void* to);
    int dataSize() const;
    uint16_t bits2pixel();
    void pixel2bits();
  private:
    NumericInt<uint32_t>*       _reg[Epix10kConfigShadow::NumberOfRegisters];
    Epix10kSimpleCount*         _count;
    bool                        _test;
    bool                        _mask;
    bool                        _g;
    bool                        _ga;
    uint16_t                    _pixel;
    QButtonGroup*               _mask_gr;
    QButtonGroup*               _test_gr;
    QButtonGroup*               _g_gr;
    QButtonGroup*               _ga_gr;
    Epix10kASICdata*            _asic;
    Pds::LinkedList<Parameter>  _asicArgs[Epix10kConfigShadow::NumberOfAsics];
    Epix10kAsicSet              _asicSet;
    Pds::LinkedList<Parameter>  pList;
  };
};

#endif
