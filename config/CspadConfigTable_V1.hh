#ifndef PdsConfigDb_CspadConfigTable_V1_hh
#define PdsConfigDb_CspadConfigTable_V1_hh

#include "pdsapp/config/Parameters.hh"

class QButtonGroup;

namespace Pds_ConfigDb
{
  class CspadConfig_V1;
  namespace V1 {
    class GlobalP;
    class QuadP;
  };

  class CspadConfigTable_V1 : public Parameter {
  public:
    CspadConfigTable_V1(const CspadConfig_V1& c);
    ~CspadConfigTable_V1();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (const void* from);
    int  push  (void* to) const;
    int  dataSize() const;
    bool validate();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    const CspadConfig_V1&      _cfg;
    Pds::LinkedList<Parameter> _pList;
    V1::GlobalP*               _globalP;
    V1::QuadP*                 _quadP[4];
  };
};

#endif
