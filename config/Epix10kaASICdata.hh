#ifndef Pds_ConfigDb_Epix10kaASICdata_hh
#define Pds_ConfigDb_Epix10kaASICdata_hh

#include "pdsapp/config/Epix10kaCopyAsicDialog.hh"
#include "pdsapp/config/Parameters.hh"

namespace Pds_ConfigDb {
  class Epix10kaASICdata : public Epix10kaCopyTarget,
                           public Parameter {
  public:
    static void setColumns(unsigned);
  public:
    Epix10kaASICdata();
    ~Epix10kaASICdata();
  public:
    void copy(const Epix10kaCopyTarget& s);
  public:
    QLayout* initialize(QWidget*);
    void update();
    void flush ();
    void enable(bool v);
  public:
    int pull(const Epix10kaASIC_ConfigShadow&);
    int push(void*);
  public:
    NumericInt<uint32_t>*       _reg[Epix10kaASIC_ConfigShadow::NumberOfRegisters];
  };
};

#endif
