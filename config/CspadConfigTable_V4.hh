#ifndef PdsConfigDb_CspadConfigTable_V4_hh
#define PdsConfigDb_CspadConfigTable_V4_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsdata/psddl/cspad.ddl.h"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class CspadConfig_V4;
  class CspadGainMap;
  namespace V4 {
    class GlobalP;
    class QuadP;
  }

  class CspadConfigTable_V4 : public Parameter {
  public:
    CspadConfigTable_V4(const CspadConfig_V4& c);
    ~CspadConfigTable_V4();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (const Pds::CsPad::ConfigV4& tc);
    int  push  (void* to) const;
    int  dataSize() const;
    bool validate();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    const CspadConfig_V4&      _cfg;
    Pds::LinkedList<Parameter> _pList;
    V4::GlobalP*               _globalP;
    V4::QuadP*                 _quadP[4];
    CspadGainMap*              _gainMap;
  };

  class CspadConfigTableQ_V4 : public QObject {
    Q_OBJECT
  public:
    CspadConfigTableQ_V4(V4::GlobalP&,
		      QWidget*);
  public slots:
    void update_readout();
  private:
    V4::GlobalP& _table;
  };
};

#endif
