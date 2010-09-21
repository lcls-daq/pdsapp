#ifndef PdsConfigDb_CspadConfigTable_hh
#define PdsConfigDb_CspadConfigTable_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class CspadConfig;
  class GlobalP;
  class QuadP;

  class CspadConfigTable : public Parameter {
  public:
    CspadConfigTable(const CspadConfig& c);
    ~CspadConfigTable();
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
    const CspadConfig&         _cfg;
    Pds::LinkedList<Parameter> _pList;
    GlobalP*                   _globalP;
    QuadP*                     _quadP[4];
  };

  class CspadConfigTableQ : public QObject {
    Q_OBJECT
  public:
    CspadConfigTableQ(GlobalP&,
		      QWidget*);
  public slots:
    void update_readout();
  private:
    GlobalP& _table;
  };
};

#endif
