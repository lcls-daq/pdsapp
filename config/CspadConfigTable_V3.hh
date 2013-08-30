#ifndef PdsConfigDb_CspadConfigTable_V3_hh
#define PdsConfigDb_CspadConfigTable_V3_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsdata/psddl/cspad.ddl.h"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class CspadConfig_V3;
  class CspadGainMap;
  namespace V3 {
    class GlobalP;
    class QuadP;
  }

  class CspadConfigTable_V3 : public Parameter {
  public:
    CspadConfigTable_V3(const CspadConfig_V3& c);
    ~CspadConfigTable_V3();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (const Pds::CsPad::ConfigV3& tc);
    int  push  (void* to) const;
    int  dataSize() const;
    bool validate();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    const CspadConfig_V3&         _cfg;
    Pds::LinkedList<Parameter> _pList;
    V3::GlobalP*                   _globalP;
    V3::QuadP*                     _quadP[4];
    CspadGainMap*              _gainMap;
  };

  class CspadConfigTableQ_V3 : public QObject {
    Q_OBJECT
  public:
    CspadConfigTableQ_V3(V3::GlobalP&,
		      QWidget*);
  public slots:
    void update_readout();
  private:
    V3::GlobalP& _table;
  };
};

#endif
