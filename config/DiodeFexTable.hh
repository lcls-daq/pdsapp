#ifndef PdsConfigDb_DiodeFexTable_hh
#define PdsConfigDb_DiodeFexTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/DiodeFexItem.hh"

namespace Pds_ConfigDb
{
  class DiodeFexTable : public Parameter {
  public:
    DiodeFexTable();
    ~DiodeFexTable();
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
    DiodeFexItem*              _diode;
    Pds::LinkedList<Parameter> _pList;
  };
};

#endif
