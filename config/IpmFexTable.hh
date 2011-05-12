#ifndef PdsConfigDb_IpmFexTable_hh
#define PdsConfigDb_IpmFexTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/DiodeFexItem.hh"

namespace Pds_ConfigDb
{
  class DiodeFexItem;

  class IpmFexTable : public Parameter {
    enum {MaxDiodes = 4};
  public:
    IpmFexTable(unsigned);
    ~IpmFexTable();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    float xscale() const;
    float yscale() const;
    void  get   (int, float*, float*) const;
    void xscale(float);
    void yscale(float);
    void  set   (int, float*, float*);
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
