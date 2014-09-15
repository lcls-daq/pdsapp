#ifndef PdsConfigDb_EpixConfigP_hh
#define PdsConfigDb_EpixConfigP_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsapp/config/EpixCopyAsicDialog.hh"

namespace Pds_ConfigDb {
  class EpixSimpleCount;
  class EpixASICdata;

  class EpixConfigP : public Parameter {
  public:
    EpixConfigP();
    ~EpixConfigP();
  public:
    QLayout* initialize(QWidget*);
    void update();
    void flush ();
    void enable(bool);
  public:
    int pull(void* from);
    int push(void* to);
    int dataSize() const;
  private:
    NumericInt<uint32_t>*       _reg[EpixConfigShadow::NumberOfRegisters];
    EpixSimpleCount*            _count;
    EpixASICdata*               _asic;
    Pds::LinkedList<Parameter>  _asicArgs[EpixConfigShadow::NumberOfAsics];
    EpixAsicSet                 _asicSet;
    Pds::LinkedList<Parameter>  pList;
  };
};

#endif
