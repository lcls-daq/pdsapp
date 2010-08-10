#ifndef PdsConfigDb_IpmFexTable_hh
#define PdsConfigDb_IpmFexTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/DiodeFexItem.hh"

namespace Pds_ConfigDb
{
  class IpmFexTable : public Parameter {
    enum {MaxDiodes = 4};
  public:
    IpmFexTable();
    ~IpmFexTable();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (void* from);
    int  push  (void* to);
    int dataSize() const;
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    DiodeFexItem*              _diodes [MaxDiodes];
    NumericFloat<float>        _xscale;
    NumericFloat<float>        _yscale;
    Pds::LinkedList<Parameter> _pList;
  };
};

#endif
