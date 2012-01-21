#ifndef PdsConfigDb_Cspad2x2ConfigTable_hh
#define PdsConfigDb_Cspad2x2ConfigTable_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class Cspad2x2Config;
  class Cspad2x2GainMap;
  class GlobalP2x2;
  class QuadP2x2;

  class Cspad2x2ConfigTable : public Parameter {
  public:
    Cspad2x2ConfigTable(const Cspad2x2Config& c);
    ~Cspad2x2ConfigTable();
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
    const Cspad2x2Config&         _cfg;
    Pds::LinkedList<Parameter>    _pList;
    GlobalP2x2*                   _globalP;
    QuadP2x2*                        _quadP[1];
    Cspad2x2GainMap*              _gainMap;
  };

  class Cspad2x2ConfigTableQ : public QObject {
    Q_OBJECT
  public:
    Cspad2x2ConfigTableQ(GlobalP2x2&,
		      QWidget*);
  public slots:
    void update_readout();
  private:
    GlobalP2x2& _table;
  };
};

#endif
