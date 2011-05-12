#ifndef PdsConfigDb_DiodeFexTable_hh
#define PdsConfigDb_DiodeFexTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/DiodeFexItem.hh"

namespace Pds_ConfigDb
{
  class DiodeFexTable : public Parameter {
  public:
    DiodeFexTable(unsigned n);
    ~DiodeFexTable();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    void  pull  (float*, float*);
    void  push  (float*, float*);
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    DiodeFexItem               _diode;
    Pds::LinkedList<Parameter> _pList;
  };
};

#endif
