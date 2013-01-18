#ifndef PdsConfigDb_Cspad2x2ConfigTable_V1_hh
#define PdsConfigDb_Cspad2x2ConfigTable_V1_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/Cspad2x2GainMap.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class Cspad2x2Config_V1;
  namespace V1
  {
    class GlobalP2x2;
    class QuadP2x2;
    class QuadPotsP2x2;
  }

  class Cspad2x2ConfigTable_V1 : public Parameter {
  public:
    Cspad2x2ConfigTable_V1(const Cspad2x2Config_V1& c);
    ~Cspad2x2ConfigTable_V1();
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
    const Cspad2x2Config_V1&      _cfg;
    Pds::LinkedList<Parameter>    _pList;
    V1::GlobalP2x2*               _globalP;
    V1::QuadP2x2*                 _quadP[1];
    V1::QuadPotsP2x2*             _quadPotsP2x2;
    Cspad2x2GainMap*              _gainMap;
  };

  namespace V1
  {
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
  }
};

#endif  // PdsConfigDb_Cspad2x2ConfigTable_V1_hh
