#ifndef PdsConfigDb_CspadConfigTable_V2_hh
#define PdsConfigDb_CspadConfigTable_V2_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class CspadConfig_V2;
  class CspadGainMap;
  namespace V2 {
    class GlobalP;
    class QuadP;
  }

  class CspadConfigTable_V2 : public Parameter {
  public:
    CspadConfigTable_V2(const CspadConfig_V2& c);
    ~CspadConfigTable_V2();
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
    const CspadConfig_V2&         _cfg;
    Pds::LinkedList<Parameter> _pList;
    V2::GlobalP*                   _globalP;
    V2::QuadP*                     _quadP[4];
    CspadGainMap*              _gainMap;
  };

  class CspadConfigTableQ_V2 : public QObject {
    Q_OBJECT
  public:
    CspadConfigTableQ_V2(V2::GlobalP&,
		      QWidget*);
  public slots:
    void update_readout();
  private:
    V2::GlobalP& _table;
  };
};

#endif
